#include <autoware_map_lane_planner/autoware_map_lane_planner.h>

AutowareMapLanePlanner::AutowareMapLanePlanner(ros::NodeHandle nh,ros::NodeHandle pnh)
{
    nh_ = nh;
    pnh_ = pnh;
    pnh_.param<double>("search_radius", search_radius_, 10.0);
    closest_waypoint_pub_ = nh_.advertise<std_msgs::Int32>("/closest_waypoint",10);
    current_pose_sub_ = nh_.subscribe("/current_pose",10,&AutowareMapLanePlanner::currentPoseCallback,this);
}

AutowareMapLanePlanner::~AutowareMapLanePlanner()
{
    
}

boost::optional<autoware_map_msgs::Waypoint> AutowareMapLanePlanner::findClosestWaypoint()
{
    std::function<bool(autoware_map_msgs::Waypoint)> filter_func_ 
        = std::bind(&AutowareMapLanePlanner::findClosestWaypointCandidates,this,std::placeholders::_1);
    std::vector<autoware_map_msgs::Waypoint> candidates = autoware_map_.findByFilter(filter_func_);
    std::vector<double> dists;
    // if there are no points in search_radius, return booost::none;
    if(candidates.size() == 0)
    {
        ROS_ERROR_STREAM("Closest waypoint does not found.");
        return boost::none;
    }
    // if there are some points in search_radius return closest waypoint
    for(auto waypoint_itr = candidates.begin(); waypoint_itr != candidates.end(); waypoint_itr++)
    {
        autoware_map::Key<autoware_map_msgs::Point> point_key(waypoint_itr->point_id);
        autoware_map::Key<autoware_map_msgs::WaypointRelation> waypoint_relation_key(waypoint_itr->waypoint_id);
        autoware_map_msgs::Point point = autoware_map_.findByKey(point_key);
        autoware_map_msgs::WaypointRelation relation = autoware_map_.findByKey(waypoint_relation_key);
        double dist = std::sqrt(std::pow(point.x-current_pose_.pose.position.x,2)
            +std::pow(point.y-current_pose_.pose.position.y,2)+std::pow(point.z-current_pose_.pose.position.z,2));
        dists.push_back(dist);
    }
    std::vector<double>::iterator min_itr = std::min_element(dists.begin(), dists.end());
    return candidates[std::distance(dists.begin(), min_itr)];
}

void AutowareMapLanePlanner::currentPoseCallback(const geometry_msgs::PoseStampedConstPtr msg)
{
    current_pose_ = *msg;
    boost::optional<autoware_map_msgs::Waypoint> closest_waypoint = findClosestWaypoint();
}

bool AutowareMapLanePlanner::findClosestWaypointCandidates(autoware_map_msgs::Waypoint waypoint)
{
    autoware_map::Key<autoware_map_msgs::Point> point_key(waypoint.point_id);
    autoware_map::Key<autoware_map_msgs::WaypointRelation> waypoint_relation_key(waypoint.waypoint_id);
    autoware_map_msgs::Point point = autoware_map_.findByKey(point_key);
    autoware_map_msgs::WaypointRelation relation = autoware_map_.findByKey(waypoint_relation_key);
    double dist = std::sqrt(std::pow(point.x-current_pose_.pose.position.x,2)
        +std::pow(point.y-current_pose_.pose.position.y,2)+std::pow(point.z-current_pose_.pose.position.z,2));
    double r,p,y;
    getRPY(current_pose_.pose.orientation,r,p,y);
    double diff_angle = getDiffAngle(y,relation.yaw);
    if(dist < search_radius_ && std::fabs(diff_angle) < (M_PI/2))
    {
        return true;
    }
    return false;
}

double AutowareMapLanePlanner::getDiffAngle(double from,double to)
{
    double ans = 0;
    double inner_prod = std::cos(from)*std::cos(to)+std::sin(from)*std::sin(to);
    double theta = std::acos(inner_prod);
    double outer_prod = std::cos(from)*std::sin(to)-std::sin(from)*std::cos(to);
    if(outer_prod > 0)
    {
        ans = theta;
    }
    else
    {
        ans = -1 * theta;
    }
    return ans;
}

void AutowareMapLanePlanner::getRPY(geometry_msgs::Quaternion q, double &roll, double &pitch, double &yaw)
{
    tf2::Quaternion quat(q.x, q.y, q.z, q.w);
    tf2::Matrix3x3(quat).getRPY(roll, pitch, yaw);
    return;
}