# TaskTrack interaction lifecycle

TaskTrack separates **human answers** from **human-to-agent assistance**. That distinction is the main rule behind the live workflow.

## Terminal task state

Only two main task states are terminal:

- `completed` — required human answers have been explicitly completed;
- `closed` — the human-facing task has ended without needing more human interaction.

`closed` can mean cancellation, or it can mean that the human explicitly delegated the remaining decision to the agent.

## Direct answer

```text
create_task
    -> GUI opens
    -> human answers
    -> Submit
    -> completed
    -> GUI closes
    -> structured answer returns to agent
```

`items[].answer.data` is the human evidence authority.

## Suggest

```text
human presses Suggest
    -> assistance request pending
    -> awaiting_agent
    -> agent returns recommendation
    -> awaiting_human
    -> recommendation appears in GUI
    -> human Accepts
    -> answer.data created
    -> completed when all required answers are ready
```

The recommendation itself is not evidence. **Accept** is the human act that turns it into evidence.

Clarify follows the same round-trip but returns explanation rather than an answer.

## Use judgement

Use judgement means “I am authorising the agent to decide this instead of asking me again.”

```text
human presses Use judgement
    -> continue_with_judgement pending
    -> awaiting_agent
    -> agent acknowledges delegation
    -> no human answer.data is created
    -> if other required human work remains: GUI stays open
    -> otherwise: task closes automatically
    -> terminal result reports delegated_to_agent=true
    -> agent continues using its own judgement
```

Delegation is durable human authority, but the resulting decision is still the agent’s judgement. It must not be reported as a human answer.

## Cancel

```text
human presses Cancel task
    -> task state closed
    -> existing human evidence is preserved
    -> no missing answer is invented
    -> GUI closes
    -> agent continues from the closed result
```

## Assistance state

While the task is active, the separate assistance channel has two effective states:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

A pending request is never terminal merely because control has temporarily moved back to the agent.

## Compatibility continuation

Some MCP hosts can service an assistance request inside the same live call. Others need to return a model turn before the agent can answer Suggest, Clarify or Use judgement.

For the latter case TaskTrack exposes pending requests explicitly. The agent resolves them with `respond_to_request` and then waits with `get_task(..., wait_ms=300000)` until the human task reaches a terminal state or requests more assistance.

From the human’s point of view the flow should remain continuous. No “done” message should be required just to wake the agent.

## Race handling

An assistance response can arrive just after the human independently completes or closes the task. The sidecar request may be marked answered so it does not remain pending forever, but terminal main-task state wins:

- no late response can reopen the task;
- no late response can alter human `answer.data`;
- no late response can resurrect `agent_action_required`.

## Invariants

1. Suggest and Clarify never create human evidence by themselves.
2. A pending `awaiting_agent` request is non-terminal.
3. Accept is explicit human evidence.
4. Use judgement is explicit delegation and never writes human `answer.data`.
5. A fully acknowledged delegation may close the human-facing task when no other required human work remains.
6. Terminal state outranks stale assistance state.
7. Durable JSON supports restart and recovery; MCP remains the normal live answer path.
8. Completion, cancellation and delegation must not require an extra human chat message to resume the agent.
