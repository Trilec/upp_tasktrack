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
