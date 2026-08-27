#include "TaskTrackApp.h"
#include <TaskTrack/Core/TaskTrackBuild.h>

using namespace Upp;

static String GuiHelpText()
{
    return
        "TaskTrack GUI\n"
        "Native human decision and verification interface.\n"
        "\n"
        "Usage:\n"
        "  TaskTrackGui.exe\n"
        "      Choose an existing TaskTrack task.\n"
        "  TaskTrackGui.exe --task <path>\n"
        "      Open the specified task.\n"
        "  TaskTrackGui.exe --help\n"
        "      Show this help.\n"
        "  TaskTrackGui.exe --version\n"
        "      Show version information.\n"
        "\n"
        "Normally launched automatically by TaskTrackMcp.exe.";
}

GUI_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();
    String task_path;
    bool agent_launch = false;

    switch(TaskTrackClassifyGuiCommand(cmd)) {
    case TaskTrackGuiCommand::Run: {
        FileSel fs;
        fs.Type("TaskTrack task", "*.tasktrack.json");
        if(!fs.ExecuteOpen("Open TaskTrack Task"))
            return;
        task_path = ~fs;
        break;
    }
    case TaskTrackGuiCommand::OpenTask:
        task_path = cmd[1];
        agent_launch = true;
        break;
    case TaskTrackGuiCommand::Help:
        PromptOK(GuiHelpText());
        return;
    case TaskTrackGuiCommand::Version:
        PromptOK(String("TaskTrack GUI\nRelease version ") + TaskTrackVersion() +
                 "\nBuild version " + TaskTrackBuildVersion());
        return;
    default:
        PromptOK(String("Unknown arguments.\n\n") + GuiHelpText());
        return;
    }

    TaskTrackWindow window;
    String error;
    if(!window.LoadTask(task_path, error)) {
        Exclamation("Unable to open TaskTrack task.\n" + error);
        return;
    }
    if(agent_launch)
        window.PrepareAgentLaunch();
    window.Run();
}
