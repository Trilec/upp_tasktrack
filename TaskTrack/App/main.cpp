#include "TaskTrackApp.h"

using namespace Upp;

GUI_APP_MAIN
{
    String task_path;
    const Vector<String>& cmd = CommandLine();

    if(cmd.GetCount() == 2 && cmd[0] == "--task")
        task_path = cmd[1];
    else if(!cmd.IsEmpty()) {
        Exclamation("Usage: TaskTrack [--task <tasktrack.json>]");
        return;
    }
    else {
        FileSel fs;
        fs.Type("TaskTrack task", "*.tasktrack.json");
        if(!fs.ExecuteOpen("Open TaskTrack Task"))
            return;
        task_path = ~fs;
    }

    TaskTrackWindow window;
    String error;
    if(!window.LoadTask(task_path, error)) {
        Exclamation("Unable to open TaskTrack task.\n" + error);
        return;
    }
    window.Run();
}
