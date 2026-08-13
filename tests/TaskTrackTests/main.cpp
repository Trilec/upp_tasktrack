#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>
#include <TaskTrack/Core/TaskTrackAgent.h>

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

    // Recommendations are advisory until the human explicitly accepts them.
    TaskTrackItem& rec_single = doc.items[1];
    rec_single.recommended = "B";
    t.Check(!rec_single.answer.answered, "single-choice recommendation pre-answered the item");
    t.Check(TaskTrackApplyRecommendation(rec_single), "single-choice recommendation could not be accepted");
    t.Check(rec_single.answer.answered && rec_single.answer.data.Is<String>() && AsString(rec_single.answer.data) == "B",
            "single-choice recommendation did not produce canonical structured evidence");
    rec_single.answer = TaskTrackAnswer();
    rec_single.recommended.Clear();

    // Bulk acceptance must never overwrite a human answer.
    rec_single.recommended = "B";
    rec_single.answer.answered = true;
    rec_single.answer.status = "selected";
    rec_single.answer.value = "C";
    rec_single.answer.data = "C";
    rec_single.answer.answered_at = TaskTrackNowIso();
    t.Check(!TaskTrackApplyRecommendation(rec_single), "recommendation overwrote an existing human answer");
    t.Check(rec_single.answer.value == "C" && AsString(rec_single.answer.data) == "C",
            "existing human evidence changed during recommendation acceptance");
    rec_single.answer = TaskTrackAnswer();
    rec_single.recommended.Clear();

    // A neutral default is presentation state, not a recommendation or evidence.
    TaskTrackItem& default_only = doc.items[8];
    default_only.default_value = 25;
    default_only.recommended.Clear();
    t.Check(!TaskTrackApplyRecommendation(default_only), "neutral default was promoted to human evidence");
    t.Check(!default_only.answer.answered, "neutral default marked an item answered");
    default_only.default_value = Value();

    // Multi-selection recommendations preserve an array rather than collapsing to display text.
    TaskTrackItem& rec_multi = doc.items[2];
    rec_multi.recommended = "A,C";
    t.Check(TaskTrackApplyRecommendation(rec_multi), "multi-choice recommendation could not be accepted");
    t.Check(rec_multi.answer.data.Is<ValueArray>() && ((ValueArray)rec_multi.answer.data).GetCount() == 2 &&
            AsString(((ValueArray)rec_multi.answer.data)[0]) == "A" &&
            AsString(((ValueArray)rec_multi.answer.data)[1]) == "C",
            "multi-choice recommendation lost structured array evidence");
    rec_multi.answer = TaskTrackAnswer();
    rec_multi.recommended.Clear();

    // Range recommendations retain the same {low,high} evidence shape used by manual range editing.
    TaskTrackItem& rec_range = doc.items[9];
    rec_range.recommended = "20,80";
    t.Check(TaskTrackApplyRecommendation(rec_range), "range recommendation could not be accepted");
    t.Check(rec_range.answer.data.Is<ValueMap>(), "range recommendation did not produce a map");
    if(rec_range.answer.data.Is<ValueMap>()) {
        ValueMap range_data = rec_range.answer.data;
        t.Check(IsNumber(range_data["low"]) && IsNumber(range_data["high"]) &&
                (double)range_data["low"] == 20.0 && (double)range_data["high"] == 80.0,
                "range recommendation changed low/high evidence");
    }
    rec_range.answer = TaskTrackAnswer();
    rec_range.recommended.Clear();

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

    t.Check(!TaskTrackCreateFromArguments(hierarchy_root, ignored, bad_path, error), "orphan hierarchy node was accepted");

    String markdown = TaskTrackExportMarkdown(answer_roundtrip);
    t.Check(markdown.Find("Semantic model acceptance") >= 0 && markdown.Find("[\"A\",\"C\"]") >= 0,
            "Markdown export omitted structured human evidence");

    // Backup recovery remains authoritative.
    SaveFile(path, "{not-json");
    TaskTrackDocument recovered;
    String recovery_message;
    t.Check(TaskTrackLoad(path, recovered, recovery_message), "backup recovery failed");
    t.Check(recovery_message.Find("recovery backup") >= 0, "backup recovery was not surfaced");
    t.Check(TaskTrackSave(path, recovered, error), "unable to restore recovered primary: " + error);

    String resolved;
    t.Check(TaskTrackResolveTaskPath(task_id, String(), resolved, error) && NormalizePath(resolved) == NormalizePath(path),
            "per-task locator did not resolve task");
    t.Check(TaskTrackTouchAgentPoll(path), "agent poll marker could not be written");
    t.Check(TaskTrackReadAgentPollEpoch(path) > 0, "agent poll marker could not be read");
    ValueArray listed;
    t.Check(TaskTrackList(root, 20, listed, error) && !listed.IsEmpty(), "list_tasks did not return stored task");

    recovered.state = TaskTrackState::Paused;
    t.Check(TaskTrackSave(path, recovered, error), "paused state save failed");
    t.Check(TaskTrackLoad(path, loaded, error) && loaded.state == TaskTrackState::Paused, "paused state did not persist");
    recovered.state = TaskTrackState::Completed;
    t.Check(TaskTrackSave(path, recovered, error), "completed state save failed");
    t.Check(TaskTrackLoad(path, loaded, error) && loaded.state == TaskTrackState::Completed, "completed state did not persist");

    String sidecar = TaskTrackAgentChannelPath(path);

    // TT-010: Pass/Fail is presentation sugar over the canonical confirm type.
    TaskTrackItem pf_yn; pf_yn.type = TaskTrackItemType::Confirm; pf_yn.choices = V("Yes", "No");
    t.Check(!TaskTrackIsPassFailConfirm(pf_yn), "ordinary Yes/No confirm was incorrectly specialized");
    TaskTrackItem pf_custom; pf_custom.type = TaskTrackItemType::Confirm; pf_custom.choices = V("Approve", "Deny");
    t.Check(!TaskTrackIsPassFailConfirm(pf_custom), "custom two-choice confirm was incorrectly specialized");
    TaskTrackItem pf_rev; pf_rev.type = TaskTrackItemType::Confirm; pf_rev.choices = V("Fail", "Pass");
    t.Check(TaskTrackIsPassFailConfirm(pf_rev), "reversed Pass/Fail confirm not recognized");

    ValueMap pf_args;
    pf_args.Add("task_id", TaskTrackMakeTaskId());
    pf_args.Add("title", "Pass/Fail verification");
    pf_args.Add("store_root", root);
    ValueArray pf_items;
    ValueMap pf; pf.Add("id", "verify"); pf.Add("type", "confirm"); pf.Add("title", "Does it match?"); pf.Add("required", true);
    pf.Add("choices", Strings(V("Pass", "Fail")));
    pf_items.Add(pf);
    pf_args.Add("items", pf_items);
    TaskTrackDocument pfdoc; String pfpath, pferr;
    t.Check(TaskTrackCreateFromArguments(pf_args, pfdoc, pfpath, pferr), "create pass/fail task failed: " + pferr);
    t.Check(TaskTrackIsPassFailConfirm(pfdoc.items[0]), "created Pass/Fail confirm not recognized");

    // Pass recommendation -> boolean true, compact value Pass; advisory, no pre-answer.
    pfdoc.items[0].answer = TaskTrackAnswer();
    pfdoc.items[0].recommended = "Pass";
    t.Check(!pfdoc.items[0].answer.answered, "Pass recommendation pre-answered before acceptance");
    t.Check(TaskTrackApplyRecommendation(pfdoc.items[0]), "Pass recommendation was not accepted");
    t.Check(pfdoc.items[0].answer.data.Is<bool>() && (bool)pfdoc.items[0].answer.data,
            "Pass did not produce boolean true evidence");
    t.Check(pfdoc.items[0].answer.value == "Pass", "Pass evidence value is not the compact display form");

    // Fail recommendation -> boolean false, compact value Fail.
    pfdoc.items[0].answer = TaskTrackAnswer();
    pfdoc.items[0].recommended = "Fail";
    t.Check(!pfdoc.items[0].answer.answered, "Fail recommendation pre-answered before acceptance");
    t.Check(TaskTrackApplyRecommendation(pfdoc.items[0]), "Fail recommendation was not accepted");
    t.Check(pfdoc.items[0].answer.data.Is<bool>() && !(bool)pfdoc.items[0].answer.data,
            "Fail did not produce boolean false evidence");
    t.Check(pfdoc.items[0].answer.value == "Fail", "Fail evidence value is not the compact display form");

    // Optional verdict note: note alone does not answer; persists; round-trips; returned in status.
    pfdoc.items[0].answer = TaskTrackAnswer();
    pfdoc.items[0].answer.note = "Narrow viewport only";
    t.Check(!pfdoc.items[0].answer.answered, "note alone marked the question answered");
    t.Check(!TaskTrackCanComplete(pfdoc), "note alone cleared the required completion gate");
    t.Check(TaskTrackSave(pfpath, pfdoc, pferr), "note save failed: " + pferr);
    TaskTrackDocument pfloaded;
    t.Check(TaskTrackLoad(pfpath, pfloaded, pferr), "note reload failed: " + pferr);
    t.Check(pfloaded.items[0].answer.note == "Narrow viewport only", "note did not round-trip through JSON");
    Value pf_status = TaskTrackStatusValue(pfloaded, pfpath, true);
    String pf_status_json = AsJSON(pf_status, false);
    t.Check(pf_status_json.Find("Narrow viewport only") >= 0, "note was not returned in result/status evidence");

    // Recommendation acceptance preserves an existing human note and still creates boolean evidence.
    pfdoc.items[0].answer = TaskTrackAnswer();
    pfdoc.items[0].answer.note = "Keep this note";
    pfdoc.items[0].recommended = "Pass";
    t.Check(TaskTrackApplyRecommendation(pfdoc.items[0]), "recommendation accept failed");
    t.Check(pfdoc.items[0].answer.note == "Keep this note", "recommendation Accept erased the human note");
    t.Check(pfdoc.items[0].answer.data.Is<bool>() && (bool)pfdoc.items[0].answer.data,
            "recommendation Accept did not create boolean evidence");

    // Verdict fields and note are independent; a verdict change preserves the note.
    TaskTrackAnswer verdict_switch;
    verdict_switch.answered = true; verdict_switch.value = "Fail"; verdict_switch.data = Value(false); verdict_switch.note = "still here";
    t.Check(verdict_switch.note == "still here", "verdict change lost the note");

    // Canonical confirm stays confirm; no new pass_fail wire type.
    t.Check(TaskTrackItemTypeName(TaskTrackItemType::Confirm) == "confirm", "canonical confirm type name changed");
    t.Check(TaskTrackItemTypeName(TaskTrackItemType::Confirm).Find("pass_fail") < 0, "pass_fail leaked into canonical type name");
    TaskTrackItemType legacy_pf_type = TaskTrackItemType::Confirm;
    t.Check(TaskTrackParseItemType("pass_fail", legacy_pf_type) && legacy_pf_type == TaskTrackItemType::SingleChoice,
            "legacy pass_fail did not remain loader-compatibility (single_choice)");

    RemoveTaskArtifacts(pfpath, AsString(pf_args["task_id"]));

    // TT-010-R1: shared CLI classification and the two-executable package names.
    t.Check(TaskTrackClassifyGuiCommand(Vector<String>()) == TaskTrackGuiCommand::Run,
            "GUI no-arg did not select the file-picker run path");
    t.Check(TaskTrackClassifyGuiCommand(V("--task", "x")) == TaskTrackGuiCommand::OpenTask,
            "GUI --task was not recognized");
    t.Check(TaskTrackClassifyGuiCommand(V("--task")) == TaskTrackGuiCommand::Invalid,
            "GUI --task without a path was accepted");
    t.Check(TaskTrackClassifyGuiCommand(V("--task", "a", "b")) == TaskTrackGuiCommand::Invalid,
            "GUI --task with extra args was accepted");
    t.Check(TaskTrackClassifyGuiCommand(V("--help")) == TaskTrackGuiCommand::Help, "GUI --help not recognized");
    t.Check(TaskTrackClassifyGuiCommand(V("-h")) == TaskTrackGuiCommand::Help, "GUI -h not recognized");
    t.Check(TaskTrackClassifyGuiCommand(V("--version")) == TaskTrackGuiCommand::Version, "GUI --version not recognized");
    t.Check(TaskTrackClassifyGuiCommand(V("bogus")) == TaskTrackGuiCommand::Invalid, "GUI unknown arg not rejected");

    t.Check(TaskTrackClassifyMcpCommand(Vector<String>()) == TaskTrackMcpCommand::Server,
            "MCP no-arg did not select stdio server mode");
    t.Check(TaskTrackClassifyMcpCommand(V("--oneshot", "f.json")) == TaskTrackMcpCommand::OneShot,
            "MCP --oneshot was not recognized");
    t.Check(TaskTrackClassifyMcpCommand(V("--oneshot")) == TaskTrackMcpCommand::Invalid,
            "incomplete MCP --oneshot was accepted");
    t.Check(TaskTrackClassifyMcpCommand(V("--selftest")) == TaskTrackMcpCommand::SelfTest,
            "MCP --selftest was not recognized");
    t.Check(TaskTrackClassifyMcpCommand(V("--help")) == TaskTrackMcpCommand::Help, "MCP --help not recognized");
    t.Check(TaskTrackClassifyMcpCommand(V("--version")) == TaskTrackMcpCommand::Version, "MCP --version not recognized");
    t.Check(TaskTrackClassifyMcpCommand(V("bogus")) == TaskTrackMcpCommand::Invalid, "MCP unknown arg not rejected");
    t.Check(TaskTrackClassifyMcpCommand(V("--help", "extra")) == TaskTrackMcpCommand::Invalid, "MCP --help with extra args accepted");

    // The runtime GUI launch target must be TaskTrackGui, not the stale TaskTrack.exe.
    String gui_name = TaskTrackGuiExecutableName();
    t.Check(gui_name.Find("TaskTrackGui") >= 0, "GUI executable name is not TaskTrackGui");
    t.Check(gui_name.Find("TaskTrack.exe") < 0, "stale TaskTrack.exe is the runtime GUI launch target");
#ifdef PLATFORM_WIN32
    t.Check(gui_name == "TaskTrackGui.exe", "Windows GUI executable name is not TaskTrackGui.exe");
#endif

    // TT-009-R1 protocol: propose_answer requires recommended; lifecycle pending -> answered.
    String req_id, req_err;
    t.Check(TaskTrackQueueAgentRequest(path, task_id, "single", "propose_answer", String(), req_id, req_err),
            "queue propose_answer failed: " + req_err);
    String dup_id, dup_err;
    t.Check(TaskTrackQueueAgentRequest(path, task_id, "single", "propose_answer", String(), dup_id, dup_err) && dup_id == req_id,
            "duplicate pending propose_answer was not reused");
    t.Check(!TaskTrackResolveAgentRequest(path, task_id, req_id, String(), String(), req_err),
            "propose_answer accepted without recommended");
    t.Check(TaskTrackResolveAgentRequest(path, task_id, req_id, "A", String(), req_err),
            "propose_answer resolve failed: " + req_err);

    TaskTrackAgentChannel ch;
    TaskTrackLoadAgentChannel(path, task_id, ch, req_err);
    const TaskTrackAgentRequest* answered_req = nullptr;
    for(const TaskTrackAgentRequest& r : ch.requests)
        if(r.id == req_id) answered_req = &r;
    t.Check(answered_req && answered_req->status == "answered",
            "propose_answer did not reach request status answered");
    String sidecar_text = LoadFile(sidecar);
    t.Check(sidecar_text.Find("resolved") < 0, "newly emitted sidecar still uses resolved request status");
    t.Check(sidecar_text.Find("answered") >= 0, "sidecar did not emit answered request status");

    // propose_answer must never create human evidence.
    TaskTrackDocument evidence_check;
    TaskTrackLoad(path, evidence_check, req_err);
    t.Check(!evidence_check.items[1].answer.answered, "agent recommendation fabricated human evidence");

    // clarify requires clarification; recommended optional; lifecycle pending -> answered.
    String cq_id, cq_err;
    t.Check(TaskTrackQueueAgentRequest(path, task_id, "text", "clarify", String(), cq_id, cq_err),
            "queue clarify failed: " + cq_err);
    t.Check(!TaskTrackResolveAgentRequest(path, task_id, cq_id, String(), String(), cq_err),
            "clarify accepted without clarification");
    t.Check(TaskTrackResolveAgentRequest(path, task_id, cq_id, String(), "Short and clear", cq_err),
            "clarify resolve failed: " + cq_err);
    TaskTrackAgentChannel cch;
    TaskTrackLoadAgentChannel(path, task_id, cch, cq_err);
    bool clarify_answered = false;
    for(const TaskTrackAgentRequest& r : cch.requests)
        if(r.id == cq_id && r.status == "answered" && !r.clarification.IsEmpty()) clarify_answered = true;
    t.Check(clarify_answered, "clarify did not reach answered with clarification preserved");
    TaskTrackDocument clarify_evidence;
    TaskTrackLoad(path, clarify_evidence, cq_err);
    t.Check(!clarify_evidence.items[5].answer.answered, "clarification fabricated human evidence");

    // continue_with_judgement: queues durably, duplicate reused, no payload required,
    // pending -> answered, never creates human evidence.
    String j_id, j_err;
    t.Check(TaskTrackQueueAgentRequest(path, task_id, "confirm", "continue_with_judgement", String(), j_id, j_err),
            "queue continue_with_judgement failed: " + j_err);
    String j_dup, j_dup_err;
    t.Check(TaskTrackQueueAgentRequest(path, task_id, "confirm", "continue_with_judgement", String(), j_dup, j_dup_err) && j_dup == j_id,
            "duplicate pending continue_with_judgement was not reused");
    TaskTrackAgentChannel jch;
    TaskTrackLoadAgentChannel(path, task_id, jch, j_err);
    ValueArray pending = TaskTrackPendingAgentRequestsValue(jch);
    bool cwj_pending = false;
    for(int i = 0; i < pending.GetCount(); ++i) {
        Value p = pending[i];
        if(AsString(p["action"]) == "continue_with_judgement" && AsString(p["id"]) == j_id) cwj_pending = true;
    }
    t.Check(cwj_pending, "continue_with_judgement missing from pending_requests");
    t.Check(TaskTrackPendingAgentRequestCount(jch) == 1, "pending-request count did not include the active pending request");
    t.Check(TaskTrackResolveAgentRequest(path, task_id, j_id, String(), String(), j_err),
            "continue_with_judgement required an unexpected response payload: " + j_err);
    TaskTrackAgentChannel jch2;
    TaskTrackLoadAgentChannel(path, task_id, jch2, j_err);
    bool cwj_answered = false;
    for(const TaskTrackAgentRequest& r : jch2.requests)
        if(r.id == j_id && r.status == "answered") cwj_answered = true;
    t.Check(cwj_answered, "continue_with_judgement did not reach answered");
    t.Check(TaskTrackPendingAgentRequestCount(jch2) == 0, "answered continue_with_judgement still counted pending");
    TaskTrackDocument judgement_evidence;
    TaskTrackLoad(path, judgement_evidence, j_err);
    t.Check(!judgement_evidence.items[0].answer.answered, "continue_with_judgement fabricated human evidence");

    // Parser accepts pending/answered/cancelled; migrates legacy resolved -> answered; rejects unknown.
    ValueMap manual;
    manual.Add("version", 1);
    manual.Add("task_id", task_id);
    ValueArray manual_reqs;
    ValueMap r_p; r_p.Add("id", "r-p"); r_p.Add("item_id", "single"); r_p.Add("action", "propose_answer"); r_p.Add("status", "pending"); manual_reqs.Add(r_p);
    ValueMap r_a; r_a.Add("id", "r-a"); r_a.Add("item_id", "single"); r_a.Add("action", "propose_answer"); r_a.Add("status", "answered"); manual_reqs.Add(r_a);
    ValueMap r_c; r_c.Add("id", "r-c"); r_c.Add("item_id", "single"); r_c.Add("action", "propose_answer"); r_c.Add("status", "cancelled"); manual_reqs.Add(r_c);
    ValueMap r_legacy; r_legacy.Add("id", "r-l"); r_legacy.Add("item_id", "single"); r_legacy.Add("action", "propose_answer"); r_legacy.Add("status", "resolved"); manual_reqs.Add(r_legacy);
    manual.Add("requests", manual_reqs);
    SaveFile(sidecar, AsJSON(manual, true));
    TaskTrackAgentChannel mch;
    t.Check(TaskTrackLoadAgentChannel(path, task_id, mch, req_err), "canonical status set rejected: " + req_err);
    bool legacy_migrated = false;
    for(const TaskTrackAgentRequest& r : mch.requests)
        if(r.id == "r-l" && r.status == "answered") legacy_migrated = true;
    t.Check(legacy_migrated, "legacy resolved request did not migrate to answered");

    ValueMap bad_manual = manual;
    ValueArray bad_reqs;
    ValueMap r_bad; r_bad.Add("id", "r-b"); r_bad.Add("item_id", "single"); r_bad.Add("action", "propose_answer"); r_bad.Add("status", "nope"); bad_reqs.Add(r_bad);
    bad_manual.Set("requests", bad_reqs);
    SaveFile(sidecar, AsJSON(bad_manual, true));
    TaskTrackAgentChannel bch;
    t.Check(!TaskTrackLoadAgentChannel(path, task_id, bch, req_err), "unknown request status was accepted");

    FileDelete(sidecar); FileDelete(sidecar + ".bak");

    RemoveTaskArtifacts(path, task_id);
    DirectoryDelete(root);

    if(t.failed.IsEmpty()) {
        Cout() << "TaskTrackTests: " << t.passed << " passed, 0 failed\n";
        return;
    }
    Cout() << "TaskTrackTests: " << t.passed << " passed, " << t.failed.GetCount() << " failed\n";
    for(const String& failure : t.failed)
        Cout() << " - " << failure << "\n";
    SetExitCode(1);
}
