#ifndef _TaskTrack_DashboardCore_TaskTrackDashboardCore_h_
#define _TaskTrack_DashboardCore_TaskTrackDashboardCore_h_

/*
    TaskTrack Dashboard Core
    ========================

    Durable semantic project-state model used by the TaskTrack dashboard
    companion. Dashboard data is presentation/management state; it is kept
    deliberately separate from TaskTrackDocument and human answer.data.

    Current state is stored as one validated JSON document. Every accepted
    update also writes an immutable numbered revision snapshot. Existing
    dashboards require base_revision on update so concurrent agents cannot
    silently overwrite newer project truth.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the GNU General Public License, version 3. See LICENSE.
*/

#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

namespace Upp {

static const int TASKTRACK_DASHBOARD_SCHEMA_VERSION = 1;
static const int TASKTRACK_DASHBOARD_MAX_PANELS = 64;
static const int TASKTRACK_DASHBOARD_MAX_ENTRIES_PER_PANEL = 1000;

enum class TaskTrackDashboardPanelType : byte {
    ProjectState,
    Timeline,
    ProgressList,
    ActionList,
    Attention,
    Verification,
    Changes,
    Records,
};

enum class TaskTrackDashboardDensity : byte {
    Summary,
    Standard,
    Full,
};

struct TaskTrackDashboardEntry : Moveable<TaskTrackDashboardEntry> {
    String id;
    String title;
    String subtitle;
    String status;
    String detail;
    String category;
    double progress = -1.0;
    double weight = 1.0;
    bool attention = false;
    bool archived = false;
    String timestamp;
    String task_id;
    String evidence;
    Value data;
};

struct TaskTrackDashboardPanel : Moveable<TaskTrackDashboardPanel> {
    String id;
    TaskTrackDashboardPanelType type = TaskTrackDashboardPanelType::Records;
    String title;
    String subtitle;
    String category;
    TaskTrackDashboardDensity density = TaskTrackDashboardDensity::Standard;
    int preview_limit = 6;
    bool contributes_to_progress = false;
    Vector<TaskTrackDashboardEntry> entries;
};

struct TaskTrackDashboardDocument : Moveable<TaskTrackDashboardDocument> {
    int schema_version = TASKTRACK_DASHBOARD_SCHEMA_VERSION;
    String dashboard_id;
    String project;
    String title;
    String subtitle;
    String phase;
    double overall_progress = -1.0;
    double confidence = -1.0;
    int revision = 0;
    String created_at;
    String updated_at;
    String updated_by;
    String source_head;
    Vector<TaskTrackDashboardPanel> panels;
};

String TaskTrackDashboardVersion();
String TaskTrackDashboardPanelTypeName(TaskTrackDashboardPanelType type);
bool TaskTrackDashboardParsePanelType(const String& text, TaskTrackDashboardPanelType& type);
String TaskTrackDashboardDensityName(TaskTrackDashboardDensity density);
bool TaskTrackDashboardParseDensity(const String& text, TaskTrackDashboardDensity& density);

String TaskTrackDashboardMakeId();
String TaskTrackDashboardDefaultStoreRoot();
String TaskTrackDashboardMakePath(const String& store_root, const String& dashboard_id);
String TaskTrackDashboardRevisionFolder(const String& store_root, const String& dashboard_id);
String TaskTrackDashboardRevisionPath(const String& store_root, const String& dashboard_id, int revision);

Value TaskTrackDashboardEntryToValue(const TaskTrackDashboardEntry& entry);
Value TaskTrackDashboardPanelToValue(const TaskTrackDashboardPanel& panel);
Value TaskTrackDashboardToValue(const TaskTrackDashboardDocument& doc);
String TaskTrackDashboardToJson(const TaskTrackDashboardDocument& doc, bool pretty = true);

bool TaskTrackDashboardFromValue(const Value& value, TaskTrackDashboardDocument& doc, String& error);
bool TaskTrackDashboardValidate(const TaskTrackDashboardDocument& doc, String& error);

bool TaskTrackDashboardSaveCurrent(const String& path, const TaskTrackDashboardDocument& doc, String& error);
bool TaskTrackDashboardLoad(const String& path, TaskTrackDashboardDocument& doc, String& error);
bool TaskTrackDashboardResolvePath(const String& dashboard_id, const String& store_root,
                                   String& path, String& error);

bool TaskTrackDashboardUpsertFromArguments(const Value& args,
                                           TaskTrackDashboardDocument& doc,
                                           String& path,
                                           String& error,
                                           String& error_code);

bool TaskTrackDashboardLoadRevision(const String& dashboard_id, const String& store_root,
                                    int revision, TaskTrackDashboardDocument& doc,
                                    String& path, String& error);
bool TaskTrackDashboardList(const String& store_root, int limit, ValueArray& result, String& error);
bool TaskTrackDashboardListRevisions(const String& dashboard_id, const String& store_root,
                                     int limit, ValueArray& result, String& error);

double TaskTrackDashboardDerivedProgress(const TaskTrackDashboardDocument& doc);
double TaskTrackDashboardEffectiveProgress(const TaskTrackDashboardDocument& doc);
int TaskTrackDashboardAttentionCount(const TaskTrackDashboardDocument& doc);
Vector<String> TaskTrackDashboardCategories(const TaskTrackDashboardDocument& doc);

Value TaskTrackDashboardStatusValue(const TaskTrackDashboardDocument& doc,
                                    const String& path,
                                    bool include_panels = true);

} // namespace Upp
#endif
