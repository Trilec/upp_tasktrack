#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

using namespace Upp;

namespace {

struct TestState {
    int passed = 0;
    Vector<String> failed;

    void Check(bool condition, const String& message)
    {
        if(condition)
            ++passed;
        else
            failed.Add(message);
    }
};

Value MakeCreateArgs(const String& root, const String& task_id = String())
{
    ValueMap check;
    check.Add("id", "check-1");
    check.Add("category", "Visual");
    check.Add("type", "check");
    check.Add("title", "Window visible");
    check.Add("required", true);

    ValueMap choice;
    choice.Add("id", "choice-1");
    choice.Add("category", "Visual");
    choice.Add("type", "choice");
    choice.Add("title", "Density");
    ValueArray choices;
    choices.Add("Compact");
    choices.Add("Spacious");
    choice.Add("choices", choices);
    choice.Add("required", false);

    ValueArray items;
    items.Add(check);
    items.Add(choice);

    ValueMap args;
    if(!task_id.IsEmpty())
        args.Add("task_id", task_id);
    args.Add("project", "TaskTrackTests");
    args.Add("title", "Core acceptance");
    args.Add("store_root", root);
    args.Add("items", items);
    return Value(args);
}

void RemoveTaskArtifacts(const String& path, const String& task_id)
{
    FileDelete(path);
    FileDelete(path + ".bak");
    FileDelete(path + ".tmp");
    FileDelete(TaskTrackPollMarkerPath(path));
    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path");
    FileDelete(locator);
    FileDelete(locator + ".bak");
    FileDelete(locator + ".tmp");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestState t;
    String root = AppendFileName(GetFileFolder(GetExeFilePath()), "_tasktrack_tests");
    RealizeDirectory(root);

    TaskTrackDocument doc;
    String path;
    String error;
    t.Check(TaskTrackCreateFromArguments(MakeCreateArgs(root), doc, path, error), "create_task failed: " + error);
    String task_id = doc.task_id;
    t.Check(FileExists(path), "task was not persisted");
    t.Check(TaskTrackStateName(doc.state) == "awaiting_human", "new task state is wrong");
    t.Check(TaskTrackRequiredCount(doc) == 1, "required count is wrong");
    t.Check(TaskTrackAnsweredCount(doc) == 0, "new task should be unanswered");
    t.Check(!TaskTrackCanComplete(doc), "unanswered required task should not complete");

    String resolved;
    t.Check(TaskTrackResolveTaskPath(task_id, String(), resolved, error) && NormalizePath(resolved) == NormalizePath(path),
            "per-task registry locator did not resolve task");

    TaskTrackDocument loaded;
    t.Check(TaskTrackLoad(path, loaded, error), "round-trip load failed: " + error);
    t.Check(loaded.items.GetCount() == 2, "round-trip item count changed");
    t.Check(loaded.items[1].choices.GetCount() == 2, "choice values were not preserved");

    loaded.items[0].answer.answered = true;
    loaded.items[0].answer.status = "confirmed";
    loaded.items[0].answer.value = "true";
    loaded.items[0].answer.answered_at = TaskTrackNowIso();
    loaded.updated_at = TaskTrackNowIso();
    t.Check(TaskTrackSave(path, loaded, error), "second save failed: " + error);
    t.Check(FileExists(path + ".bak"), "second save did not create recovery backup");
    t.Check(TaskTrackCanComplete(loaded), "answered required task should complete");
    t.Check(TaskTrackRequiredAnsweredCount(loaded) == 1, "required answered count is wrong");

    String markdown = TaskTrackExportMarkdown(loaded);
    t.Check(markdown.Find("Window visible") >= 0 && markdown.Find("confirmed") >= 0,
            "Markdown export omitted evidence");

    // Corrupt primary after a valid backup exists; loader must recover from backup.
    SaveFile(path, "{not-json");
    TaskTrackDocument recovered;
    String recovery_message;
    t.Check(TaskTrackLoad(path, recovered, recovery_message), "backup recovery failed");
    t.Check(recovery_message.Find("recovery backup") >= 0, "backup recovery was not surfaced");

    // Strict persisted type validation: malformed choices are rejected, not silently defaulted.
    Value invalid_value = TaskTrackToValue(recovered);
    ValueArray invalid_items = invalid_value["items"];
    ValueMap first = invalid_items[0];
    first.Set("choices", "not-an-array");
    invalid_items.Set(0, first);
    ValueMap invalid_root = invalid_value;
    invalid_root.Set("items", invalid_items);
    TaskTrackDocument ignored;
    String invalid_error;
    t.Check(!TaskTrackFromValue(invalid_root, ignored, invalid_error) && invalid_error.Find("choices must be an array") >= 0,
            "malformed persisted choices were accepted");

    Value bad_item_args = MakeCreateArgs(root, TaskTrackMakeTaskId());
    ValueArray bad_items = bad_item_args["items"];
    ValueMap bad_choice = bad_items[1];
    bad_choice.Set("choices", "wrong");
    bad_items.Set(1, bad_choice);
    ValueMap bad_root = bad_item_args;
    bad_root.Set("items", bad_items);
    String bad_path;
    TaskTrackDocument bad_doc;
    t.Check(!TaskTrackCreateFromArguments(bad_root, bad_doc, bad_path, invalid_error),
            "create_task accepted non-array choices");

    // Poll marker is an independent signal and must not mutate task evidence.
    t.Check(TaskTrackTouchAgentPoll(path), "agent poll marker could not be written");
    t.Check(TaskTrackReadAgentPollEpoch(path) > 0, "agent poll marker could not be read");

    // list_tasks returns stored evidence when primary is repaired.
    t.Check(TaskTrackSave(path, recovered, error), "unable to repair recovered primary");
    ValueArray listed;
    t.Check(TaskTrackList(root, 20, listed, error), "list_tasks failed");
    t.Check(!listed.IsEmpty(), "list_tasks returned no tasks");

    // Completion and explicit close are persisted states.
    recovered.state = TaskTrackState::Completed;
    recovered.updated_at = TaskTrackNowIso();
    t.Check(TaskTrackSave(path, recovered, error), "completed state save failed");
    t.Check(TaskTrackLoad(path, loaded, error) && loaded.state == TaskTrackState::Completed,
            "completed state did not round-trip");

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
