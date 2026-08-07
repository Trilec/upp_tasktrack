    String task_path = TaskTrackMakeTaskPath(root, task_id);
    st.Check(FileExists(task_path), "create_task returned before durable task existed");

    ValueMap task_params; task_params.Add("taskId", task_id); task_params.Add("_meta", ModernMeta(true));
    String get_json = AsJSON(HandleRequest(Request("tasks/get", 5, task_params), has), false);
    st.Check(get_json.Find("\"status\":\"working\"") >= 0, "tasks/get did not report working");

    TaskTrackDocument doc; String error;
    if(TaskTrackLoad(task_path, doc, error)) {
        doc.items[0].answer.answered = true; doc.items[0].answer.status = "selected"; doc.items[0].answer.value = "Pass";
        doc.items[0].answer.data = "Pass"; doc.items[0].answer.answered_at = TaskTrackNowIso(); doc.state = TaskTrackState::Completed; doc.updated_at = TaskTrackNowIso();
        st.Check(TaskTrackSave(task_path, doc, error), "unable to complete self-test task");
    }
    else st.Check(false, "unable to load self-test task: " + error);

    get_json = AsJSON(HandleRequest(Request("tasks/get", 6, task_params), has), false);
    st.Check(get_json.Find("\"status\":\"completed\"") >= 0, "tasks/get did not report completed");
    st.Check(get_json.Find("structuredContent") >= 0 && get_json.Find("\"data\":\"Pass\"") >= 0,
             "completed task missing structured human result");

    ValueMap bad_meta; ValueMap meta; meta.Add("io.modelcontextprotocol/protocolVersion", "2026-01-01"); meta.Add("io.modelcontextprotocol/clientCapabilities", ValueMap()); bad_meta.Add("_meta", meta);
    String bad_json = AsJSON(HandleRequest(Request("tools/list", 7, bad_meta), has), false);
    st.Check(bad_json.Find("-32022") >= 0, "unsupported protocol revision was not rejected");

    FileDelete(task_path); FileDelete(task_path + ".bak"); FileDelete(TaskTrackPollMarkerPath(task_path));
    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path"); FileDelete(locator); FileDelete(locator + ".bak"); DirectoryDelete(root);

    if(st.failures.IsEmpty()) { Cout() << "tasktrack-mcp-selftest: ok\n"; return 0; }
    Cout() << "tasktrack-mcp-selftest: failed\n";
    for(const String& failure : st.failures) Cout() << " - " << failure << "\n";
    return 1;
}

} // namespace

CONSOLE_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();
    if(cmd.GetCount() == 1 && cmd[0] == "--selftest") { SetExitCode(RunSelfTest()); return; }
    if(cmd.GetCount() == 2 && cmd[0] == "--oneshot") { SetExitCode(RunOneShot(cmd[1])); return; }
    if(!cmd.IsEmpty()) { Cout() << "TaskTrackMcp [--selftest] [--oneshot request.json]\n"; SetExitCode(2); return; }
    SetExitCode(RunServer());
}
