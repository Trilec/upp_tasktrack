#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

using namespace Upp;

namespace {

void AddItem(ValueArray& items, const String& id, const String& category,
             const String& type, const String& title, const String& instruction,
             bool required = true, const String& expected_value = String(),
             const String& expected_color = String(), const Vector<String>& choices = Vector<String>())
{
    ValueMap item;
    item.Add("id", id);
    item.Add("category", category);
    item.Add("type", type);
    item.Add("title", title);
    item.Add("instruction", instruction);
    item.Add("required", required);
    item.Add("expected_value", expected_value);
    item.Add("expected_color", expected_color);
    ValueArray choice_values;
    for(const String& choice : choices)
        choice_values.Add(choice);
    item.Add("choices", choice_values);
    items.Add(item);
}

} // namespace

CONSOLE_APP_MAIN
{
    ValueArray items;
    AddItem(items, "check-visible", "Startup", "check", "Application is visible",
            "Confirm the window opened and is not blank.");
    AddItem(items, "pass-resize", "Interaction", "pass_fail", "Resize works",
            "Resize from the bottom-right corner and check that content remains usable.");
    Vector<String> choices;
    choices.Add("Desktop-like");
    choices.Add("Tablet-like");
    choices.Add("Mobile-like");
    AddItem(items, "choice-density", "Layout", "choice", "Current density",
            "Choose the closest visual density.", true, String(), String(), choices);
    AddItem(items, "text-label", "Content", "text", "Read the primary title",
            "Type the exact title displayed at the top of the task.");
    AddItem(items, "notes", "Content", "multiline", "General visual notes",
            "Record any clipping, overlap, spacing, or readability problem.", false);
    AddItem(items, "number-width", "Layout", "number", "Approximate window width",
            "Enter the approximate width in pixels.", false, "about 1180 px");
    AddItem(items, "color-accent", "Visual", "color", "Accent colour matches",
            "Compare the TaskTrack accent against this expected blue.", true, String(), "#0078D4");
    AddItem(items, "file-export", "Persistence", "file", "Markdown export exists",
            "Use Save > Export Markdown and confirm the file was created.", true, "<task-id>.md");
    AddItem(items, "interaction-pause", "Interaction", "interaction", "Pause and resume",
            "Pause the task, verify editing is preserved, then resume.");
    AddItem(items, "visual-cards", "Visual", "visual_compare", "Cards align cleanly",
            "Compare card edges and wrapping across the visible task columns.");

    ValueMap args;
    args.Add("project", "TaskTrack");
    args.Add("title", "TaskTrack V0.1 visual acceptance");
    args.Add("subtitle", "Example containing every V0.1 verification field type");
    args.Add("actor", "TaskTrackExample");
    args.Add("store_root", AppendFileName(GetFileFolder(GetExeFilePath()), "tasktrack_data"));
    args.Add("reminder_minutes", 60);
    args.Add("remind_while_paused", false);
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
