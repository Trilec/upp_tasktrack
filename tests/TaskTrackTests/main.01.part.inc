#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

using namespace Upp;

namespace {

struct TestState {
    int passed = 0;
    Vector<String> failed;
    void Check(bool condition, const String& message)
    {
        if(condition) ++passed;
        else failed.Add(message);
    }
};

ValueArray Strings(const Vector<String>& values)
{
    ValueArray out;
    for(const String& value : values) out.Add(value);
    return out;
}

Vector<String> V(const char *a, const char *b = nullptr, const char *c = nullptr,
                 const char *d = nullptr, const char *e = nullptr)
{
    Vector<String> out;
    if(a) out.Add(a); if(b) out.Add(b); if(c) out.Add(c); if(d) out.Add(d); if(e) out.Add(e);
    return out;
}

ValueMap Item(const String& id, const String& type)
{
    ValueMap item;
    item.Add("id", id);
    item.Add("category", "Test");
    item.Add("type", type);
    item.Add("title", id);
    item.Add("instruction", "Test " + type);
    item.Add("required", id == "confirm");
    return item;
}

Value MakeAllTypesArgs(const String& root, const String& task_id = String())
{
    ValueArray items;
    ValueMap x;

    items.Add(Item("confirm", "confirm"));

    x = Item("single", "single_choice"); x.Add("choices", Strings(V("A", "B", "C"))); items.Add(x);
    x = Item("multi", "multi_choice"); x.Add("choices", Strings(V("A", "B", "C"))); items.Add(x);
    x = Item("select", "select"); x.Add("choices", Strings(V("A", "B", "C"))); items.Add(x);
    x = Item("list", "list_select"); x.Add("choices", Strings(V("A", "B", "C"))); x.Add("allow_multiple", true); items.Add(x);
    items.Add(Item("text", "text"));
    items.Add(Item("notes", "notes"));

    x = Item("number", "number"); x.Add("min", 1); x.Add("max", 12); x.Add("step", 1); items.Add(x);
    x = Item("amount", "amount"); x.Add("min", 0); x.Add("max", 100); x.Add("step", 5); items.Add(x);
    x = Item("range", "range"); x.Add("min", 0); x.Add("max", 100); x.Add("step", 5); items.Add(x);
    x = Item("rating", "rating"); x.Add("min", 1); x.Add("max", 5); items.Add(x);

    x = Item("color", "color"); x.Add("colors", Strings(V("#2F6FED", "#00A878"))); items.Add(x);

    x = Item("gradient", "gradient");
    ValueArray gradients;
    ValueMap g; g.Add("id", "cool"); g.Add("label", "Cool"); g.Add("from", "#112233"); g.Add("to", "#445566"); gradients.Add(g);
    x.Add("gradients", gradients); items.Add(x);

    items.Add(Item("position", "position"));
    items.Add(Item("direction", "direction"));

    x = Item("rank", "rank_order"); x.Add("choices", Strings(V("First", "Second", "Third"))); items.Add(x);

    x = Item("tree", "hierarchy_select");
    ValueArray nodes;
    ValueMap root_node; root_node.Add("id", "root"); root_node.Add("parent_id", ""); root_node.Add("label", "Root"); nodes.Add(root_node);
    ValueMap child; child.Add("id", "child"); child.Add("parent_id", "root"); child.Add("label", "Child"); nodes.Add(child);
    x.Add("hierarchy", nodes); items.Add(x);

    x = Item("curve", "curve");
    ValueArray curve; curve.Add(0.25); curve.Add(0.1); curve.Add(0.25); curve.Add(1.0); x.Add("default", curve); items.Add(x);

    ValueMap args;
    if(!task_id.IsEmpty()) args.Add("task_id", task_id);
    args.Add("project", "TaskTrackTests");
    args.Add("title", "Semantic model acceptance");
    args.Add("store_root", root);
    args.Add("items", items);
    return Value(args);
}

void RemoveTaskArtifacts(const String& path, const String& task_id)
{
    FileDelete(path); FileDelete(path + ".bak"); FileDelete(path + ".tmp");
    FileDelete(TaskTrackPollMarkerPath(path));
    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path");
    FileDelete(locator); FileDelete(locator + ".bak"); FileDelete(locator + ".tmp");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestState t;
    String root = AppendFileName(GetFileFolder(GetExeFilePath()), "_tasktrack_tests");
    RealizeDirectory(root);

    // Gary W1 regression: generated id must compile through Upp::Format, contain a literal T,
    // and never contain the old <N/A 'dT'> formatting artefact.
    String generated = TaskTrackMakeTaskId();
    t.Check(generated.StartsWith("task-"), "generated task id prefix is wrong");
    t.Check(generated.Find("T") >= 0, "generated task id lost literal T separator");
    t.Check(generated.Find("<N/A") < 0, "generated task id contains U++ format error text");

    TaskTrackDocument doc;
    String path;
    String error;
    t.Check(TaskTrackCreateFromArguments(MakeAllTypesArgs(root), doc, path, error), "create all-types task failed: " + error);
    String task_id = doc.task_id;
    t.Check(doc.schema_version == 2, "new document is not schema v2");
    t.Check(doc.items.GetCount() == 18, "semantic type demo model does not contain 18 items");
    for(int i = 0; i < doc.items.GetCount(); ++i)
        t.Check((int)doc.items[i].type == i, Format("semantic type %d did not round-trip in enum order", i));
    t.Check(FileExists(path), "task was not persisted before create returned");
    t.Check(!TaskTrackCanComplete(doc), "required confirm should initially block completion");

    // Gary W1 regression: AsJSON -> ParseJSON represents JSON numbers as generic numeric Values.
    // Saving must accept schema_version/reminder/history fields after that round trip.
    String serialized = TaskTrackToJson(doc, true);
    Value parsed_json = ParseJSON(serialized);
    t.Check(IsNumber(parsed_json["schema_version"]), "JSON schema_version is not numeric");
    TaskTrackDocument parsed_doc;
    t.Check(TaskTrackFromValue(parsed_json, parsed_doc, error), "numeric JSON round-trip rejected: " + error);
    t.Check(parsed_doc.items.GetCount() == 18, "numeric JSON round-trip lost items");

    TaskTrackDocument loaded;
    t.Check(TaskTrackLoad(path, loaded, error), "saved semantic task failed to load: " + error);
    t.Check(loaded.schema_version == 2, "loaded semantic task did not normalize to schema v2");

    // Structured answer data survives arrays/maps rather than collapsing to display text.
    ValueArray selected; selected.Add("A"); selected.Add("C");
    loaded.items[2].answer.answered = true;
    loaded.items[2].answer.status = "selected";
    loaded.items[2].answer.value = AsJSON(selected, false);
    loaded.items[2].answer.data = selected;
    loaded.items[2].answer.answered_at = TaskTrackNowIso();
    loaded.items[0].answer.answered = true;
    loaded.items[0].answer.status = "Yes";
    loaded.items[0].answer.value = "yes";
    loaded.items[0].answer.data = true;
    loaded.items[0].answer.answered_at = TaskTrackNowIso();
    t.Check(TaskTrackSave(path, loaded, error), "structured answer save failed: " + error);
    t.Check(FileExists(path + ".bak"), "second save did not create recovery backup");
    t.Check(TaskTrackCanComplete(loaded), "answered required confirm did not clear completion gate");

    TaskTrackDocument answer_roundtrip;
    t.Check(TaskTrackLoad(path, answer_roundtrip, error), "structured answer reload failed: " + error);
    t.Check(answer_roundtrip.items[2].answer.data.Is<ValueArray>(), "multi-choice answer data lost array type");
    t.Check(((ValueArray)answer_roundtrip.items[2].answer.data).GetCount() == 2, "multi-choice answer array changed");
    t.Check(answer_roundtrip.items[0].answer.data.Is<bool>() && (bool)answer_roundtrip.items[0].answer.data,
            "confirm answer lost boolean data");

    // V0.1 compatibility: old names load and normalize to canonical semantic types.
    ValueMap legacy_root;
    legacy_root.Add("schema_version", 1);
    legacy_root.Add("task_id", "task-legacy-compat");
    legacy_root.Add("title", "Legacy compatibility");
    legacy_root.Add("state", "awaiting_human");
    ValueArray legacy_items;
    ValueMap pass; pass.Add("id", "pass"); pass.Add("type", "pass_fail"); pass.Add("title", "Pass?"); legacy_items.Add(pass);
    ValueMap multi_line; multi_line.Add("id", "memo"); multi_line.Add("type", "multiline"); multi_line.Add("title", "Memo"); legacy_items.Add(multi_line);
    ValueMap color_witness;
    color_witness.Add("id", "color-witness");
    color_witness.Add("type", "color");
    color_witness.Add("title", "Expected colour matches?");
    color_witness.Add("expected_color", "#2F6FED");
    ValueMap color_answer;
    color_answer.Add("answered", true);
    color_answer.Add("status", "Match");
    color_answer.Add("value", "match");
    color_witness.Add("answer", color_answer);
    legacy_items.Add(color_witness);
    legacy_root.Add("items", legacy_items);
    TaskTrackDocument migrated;
    t.Check(TaskTrackFromValue(legacy_root, migrated, error), "V0.1 migration failed: " + error);
    t.Check(migrated.schema_version == 2, "V0.1 task was not normalized to v2");
    t.Check(migrated.items[0].type == TaskTrackItemType::SingleChoice, "pass_fail did not migrate to single_choice");
    t.Check(migrated.items[0].choices.GetCount() == 4, "pass_fail migration did not synthesize choices");
    t.Check(migrated.items[1].type == TaskTrackItemType::Notes, "multiline did not migrate to notes");
    t.Check(migrated.items[2].type == TaskTrackItemType::SingleChoice, "V0.1 color witness was reinterpreted as a color chooser");
    t.Check(migrated.items[2].choices.GetCount() == 3 && migrated.items[2].choices[0] == "Match",
            "V0.1 color witness did not synthesize verdict choices");
    t.Check(migrated.items[2].answer.data.Is<String>() && AsString(migrated.items[2].answer.data) == "Match",
            "V0.1 color witness answer did not normalize to structured verdict data");
    t.Check(migrated.items[2].instruction.Find("#2F6FED") >= 0,
            "V0.1 color witness lost expected colour context");

    // Strict validation stays authoritative.
    Value bad = MakeAllTypesArgs(root, TaskTrackMakeTaskId());
    ValueArray bad_items = bad["items"];
    ValueMap bad_gradient = bad_items[12];
    bad_gradient.Set("gradients", "not-an-array");
    bad_items.Set(12, bad_gradient);
    ValueMap bad_root = bad;
    bad_root.Set("items", bad_items);
    TaskTrackDocument ignored;
    String bad_path;
    t.Check(!TaskTrackCreateFromArguments(bad_root, ignored, bad_path, error), "malformed gradients were accepted");

    Value hierarchy_bad = MakeAllTypesArgs(root, TaskTrackMakeTaskId());
    ValueArray hierarchy_items = hierarchy_bad["items"];
    ValueMap hierarchy_item = hierarchy_items[16];
    ValueArray bad_nodes;
    ValueMap orphan; orphan.Add("id", "orphan"); orphan.Add("parent_id", "missing"); orphan.Add("label", "Orphan"); bad_nodes.Add(orphan);
    hierarchy_item.Set("hierarchy", bad_nodes);
    hierarchy_items.Set(16, hierarchy_item);
    ValueMap hierarchy_root = hierarchy_bad;
    hierarchy_root.Set("items", hierarchy_items);
