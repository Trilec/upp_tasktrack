                     idle_seconds >= 60;

    if(!periodic_due && !agent_due)
        return;

    last_seen_agent_poll_epoch_ = max(last_seen_agent_poll_epoch_, poll_epoch);
    int result = RunReminderPrompt(agent_due);
    ++document_.reminder_count;
    TouchHumanActivity();

    if(result == REMINDER_CONTINUE)
        document_.state = TaskTrackState::InProgress;
    else if(result == REMINDER_PAUSE)
        document_.state = TaskTrackState::Paused;
    else if(result == REMINDER_CLOSE)
        document_.state = TaskTrackState::Closed;

    SaveProgress(false);
    if(result == REMINDER_CLOSE)
        RebuildItems();
    RefreshHeaderState();
}

} // namespace Upp
