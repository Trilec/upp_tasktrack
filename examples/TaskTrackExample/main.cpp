#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

#include <initializer_list>

using namespace Upp;

namespace {

ValueArray Strings(std::initializer_list<const char *> values)
{
    ValueArray out;
    for(const char *value : values)
        out.Add(value);
    return out;
}

ValueMap BaseItem(const String& id, const String& category, const String& type,
                  const String& title, const String& instruction, bool required = true)
{
    ValueMap item;
    item.Add("id", id);
    item.Add("category", category);
    item.Add("type", type);
    item.Add("title", title);
    item.Add("instruction", instruction);
    item.Add("required", required);
    return item;
}

void AddChoices(ValueMap& item, std::initializer_list<const char *> values)
{
    item.Add("choices", Strings(values));
}

void AddGradient(ValueArray& gradients, const String& id, const String& label,
                 const String& from, const String& to)
{
    ValueMap g;
    g.Add("id", id);
    g.Add("label", label);
    g.Add("from", from);
    g.Add("to", to);
    gradients.Add(g);
}

void AddNode(ValueArray& hierarchy, const String& id, const String& parent,
             const String& label)
{
    ValueMap node;
    node.Add("id", id);
    node.Add("parent_id", parent);
    node.Add("label", label);
    hierarchy.Add(node);
}

} // namespace

CONSOLE_APP_MAIN
{
    ValueArray items;

    ValueMap confirm = BaseItem("confirm", "Decision", "confirm",
        "Apply this change?", "Confirm whether the agent should continue with this direction.");
    confirm.Add("recommended", "Yes");
    items.Add(confirm);

    ValueMap single = BaseItem("single-choice", "Decision", "single_choice",
        "Which implementation direction?", "Choose the overall level of intervention.");
    AddChoices(single, { "Minimal", "Balanced", "Advanced" });
    single.Add("recommended", "Balanced");
    items.Add(single);

    ValueMap multi = BaseItem("multi-choice", "Decision", "multi_choice",
        "What should this change include?", "Select every independent area that should be included.");
    AddChoices(multi, { "Layout", "Style", "Behaviour", "Tests" });
    items.Add(multi);

    ValueMap select = BaseItem("select", "Decision", "select",
        "Which source should I use?", "Choose the authoritative source for the next operation.");
    AddChoices(select, { "Current selection", "Parent container", "Theme defaults", "Project settings" });
    items.Add(select);

    ValueMap list = BaseItem("list-select", "Decision", "list_select",
        "Which outputs should be reviewed?", "Select one or more outputs from the larger visible list.");
    AddChoices(list, { "Desktop preview", "Tablet preview", "Mobile preview", "Published output", "Saved project" });
    list.Add("allow_multiple", true);
    items.Add(list);

    items.Add(BaseItem("text", "Input", "text",
        "What should the primary label say?", "Enter a short exact text response."));

    items.Add(BaseItem("notes", "Input", "notes",
        "Anything I should know?", "Add an exception, constraint, qualification, or instruction.", false));

    ValueMap number = BaseItem("number", "Input", "number",
        "How many variants?", "Enter the exact discrete number required.");
    number.Add("min", 1);
    number.Add("max", 12);
    number.Add("step", 1);
    number.Add("unit", " variants");
    items.Add(number);

    ValueMap amount = BaseItem("amount", "Input", "amount",
        "How much spacing?", "Choose the amount visually or enter the numeric value.");
    amount.Add("min", 0);
    amount.Add("max", 64);
    amount.Add("step", 4);
    amount.Add("unit", " px");
    amount.Add("default", 24);
    items.Add(amount);

    ValueMap range = BaseItem("range", "Input", "range",
        "Preferred width range?", "Set the acceptable low and high bounds.");
    range.Add("min", 160);
    range.Add("max", 1280);
    range.Add("step", 20);
    range.Add("unit", " px");
    ValueMap range_default;
    range_default.Add("low", 320);
    range_default.Add("high", 900);
    range.Add("default", range_default);
    items.Add(range);

    ValueMap rating = BaseItem("rating", "Input", "rating",
        "Rate this result", "Use a compact score for quality, confidence, fit, or urgency.");
    rating.Add("min", 1);
    rating.Add("max", 5);
    items.Add(rating);

    ValueMap color = BaseItem("color", "Visual", "color",
        "Choose the accent colour", "Pick a supplied swatch or use the custom colour control.");
    color.Add("colors", Strings({ "#2F6FED", "#7C4DFF", "#00A878", "#E26D2F" }));
    color.Add("recommended", "#2F6FED");
    items.Add(color);

    ValueMap gradient = BaseItem("gradient", "Visual", "gradient",
        "Choose the gradient treatment", "Choose visually rather than describing the appearance in words.");
    ValueArray gradients;
    AddGradient(gradients, "cool", "Cool", "#2C3E50", "#4CA1AF");
    AddGradient(gradients, "warm", "Warm", "#FF7E5F", "#FEB47B");
    AddGradient(gradients, "neutral", "Neutral", "#232526", "#6B7280");
    gradient.Add("gradients", gradients);
    gradient.Add("recommended", "cool");
    items.Add(gradient);

    ValueMap position = BaseItem("position", "Visual", "position",
        "Where should the element sit?", "Choose one of the nine spatial positions.");
    position.Add("default", "center");
    items.Add(position);

    ValueMap direction = BaseItem("direction", "Visual", "direction",
        "Where should this panel open?", "Choose the preferred opening direction.");
    direction.Add("default", "east");
    items.Add(direction);

    ValueMap rank = BaseItem("rank-order", "Structure", "rank_order",
        "Rank the implementation priorities", "Drag the rows into priority order, then accept the order.");
    AddChoices(rank, { "Correctness", "Usability", "Compactness", "Novelty" });
    items.Add(rank);

    ValueMap hierarchy = BaseItem("hierarchy", "Structure", "hierarchy_select",
        "Which component should change?", "Choose the target from the component hierarchy.");
    ValueArray nodes;
    AddNode(nodes, "app", "", "Application");
    AddNode(nodes, "header", "app", "Header");
    AddNode(nodes, "workspace", "app", "Workspace");
    AddNode(nodes, "canvas", "workspace", "Canvas");
    AddNode(nodes, "inspector", "workspace", "Inspector");
    AddNode(nodes, "footer", "app", "Footer");
    hierarchy.Add("hierarchy", nodes);
    items.Add(hierarchy);

    ValueMap curve = BaseItem("curve", "Structure", "curve",
        "How should the transition feel?", "Shape the easing/falloff curve, then accept it.");
    ValueArray bezier;
    bezier.Add(0.25); bezier.Add(0.10); bezier.Add(0.25); bezier.Add(1.00);
    curve.Add("default", bezier);
    items.Add(curve);

    ValueMap args;
    args.Add("project", "TaskTrack");
    args.Add("title", "Agent needs your input");
    args.Add("subtitle", "One compact example of every TaskTrack semantic question type");
    args.Add("actor", "TaskTrackExample");
    args.Add("store_root", AppendFileName(GetFileFolder(GetExeFilePath()), "tasktrack_data"));
    args.Add("reminder_minutes", 60);
    args.Add("nudge_on_agent_poll", true);
    args.Add("items", items);

    TaskTrackDocument doc;
    String path;
    String error;
    if(!TaskTrackCreateFromArguments(args, doc, path, error)) {
        Cerr() << "TaskTrackExample failed: " << error << "\n";
        SetExitCode(1);
        return;
    }

    Cout() << path << "\n";
}
