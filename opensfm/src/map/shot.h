#pragma once
// #include <opencv2/features2d/features2d.hpp>
// #include <opencv2/core.hpp>
#include <Eigen/Eigen>
#include <map/defines.h>
#include <map/pose.h>
#include <map/slam_shot_data.h>
#include <map/observation.h>
#include <map/camera.h>
#include <sfm/observation.h>
// #include <map/landmark.h>
namespace map
{
class Pose;
// class Camera;
class Landmark;

struct ShotCamera {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  ShotCamera(const Camera& camera, const CameraId cam_id, const std::string cam_name = ""):
    camera_model_(camera), id_(cam_id), camera_name_(cam_name){}
  const Camera& camera_model_;
  const int id_;
  const std::string camera_name_;
};

struct ShotMeasurements
{
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Eigen::Vector3d gps_;
  double capture_time_;
  //TODO:
  //compass
  // accelerometer
  double gps_dop_{0};
  std::array<double, 3> gps_position_{0};
  int orientation_;
  std::string skey;
  
};

class Shot {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Shot(const ShotId shot_id, const ShotCamera& shot_camera, const Pose& pose, const std::string& name = "");
  const DescriptorType GetDescriptor(const FeatureId id) const { return descriptors_.row(id); }
  const auto& GetKeyPoint(const FeatureId id) const { return keypoints_.at(id); }
  Eigen::Vector3f GetKeyPointEigen(const FeatureId id) const { 
    const auto kpt = keypoints_.at(id);
    return Eigen::Vector3f(kpt.point[0], kpt.point[1], kpt.size);
  }
  //No reason to set individual keypoints or descriptors
  //read-only access
  const auto& GetKeyPoints() const { return keypoints_; }
  const DescriptorMatrix& GetDescriptors() const { return descriptors_; }
  
  size_t NumberOfKeyPoints() const { return keypoints_.size(); }
  size_t ComputeNumValidLandmarks(const int min_obs_thr) const;
  float ComputeMedianDepthOfLandmarks(const bool take_abs) const;
  
  const std::vector<Landmark*>& GetLandmarks() const { return landmarks_; }
  std::vector<Landmark*>& GetLandmarks() { return landmarks_; }
  std::vector<Landmark*> ComputeValidLandmarks()
  {
    //we use the landmark observation
    if (landmarks_.empty())
    {
      std::vector<Landmark*> valid_landmarks;
      valid_landmarks.reserve(landmark_observations_.size());
      std::transform(landmark_observations_.begin(), landmark_observations_.end(),
                std::back_inserter(valid_landmarks), [](const auto& p){ return p.first; });
      return valid_landmarks;
    }
    else
    {
      std::vector<Landmark*> valid_landmarks;
      valid_landmarks.reserve(landmarks_.size());
      std::copy_if(landmarks_.begin(), landmarks_.end(),
                std::back_inserter(valid_landmarks), [](const auto* lm){ return lm != nullptr; });
      return valid_landmarks;
    }
  }
  std::vector<FeatureId> ComputeValidLandmarksIndices() const
  {
    std::vector<FeatureId> valid_landmarks;
    valid_landmarks.reserve(landmarks_.size());
    for (size_t idx = 0; idx < landmarks_.size(); ++idx)
    {
      if (landmarks_[idx] != nullptr)
      {
        valid_landmarks.push_back(idx);
      }
    }
    return valid_landmarks;
  }
  std::vector<std::pair<Landmark*,FeatureId>> ComputeValidLandmarksAndIndices() const
  {
    std::vector<std::pair<Landmark*,FeatureId>> valid_landmarks;
    valid_landmarks.reserve(landmarks_.size());
    for (size_t idx = 0; idx < landmarks_.size(); ++idx)
    {
      auto* lm = landmarks_[idx];
      if (lm != nullptr)
      {
        valid_landmarks.push_back(std::make_pair(lm,idx));
      }
    }
    return valid_landmarks;
  }
  


  Landmark* GetLandmark(const FeatureId id) { return landmarks_.at(id);}
  void RemoveLandmarkObservation(const FeatureId id) 
  { 
    landmarks_.at(id) = nullptr; 
  }
  void AddLandmarkObservation(Landmark* lm, const FeatureId feat_id) 
  { 
    landmarks_.at(feat_id) = lm; 
  }

  void CreateObservation(Landmark* lm, const Eigen::Vector2d& pt, const double scale,
                        const Eigen::Vector3i& color, FeatureId id)
  {
    landmark_observations_.insert(std::make_pair(lm, std::make_unique<Observation>(pt[0], pt[1], scale, color[0], color[1], color[2], id))); 
  }

  void SetPose(const Pose& pose) { pose_ = pose; }
  const Pose& GetPose() const { return pose_; }
  Eigen::Matrix4d GetWorldToCam() const { return pose_.WorldToCamera(); }
  Eigen::Matrix4d GetCamToWorld() const { return pose_.CameraToWorld(); }
  
  void InitAndTakeDatastructures(AlignedVector<Observation> keypts, DescriptorMatrix descriptors);
  void InitKeyptsAndDescriptors(const size_t n_keypts);
  
  // SLAM stuff
  void UndistortedKeyptsToBearings();
  void UndistortKeypts();

  void ScalePose(const float scale);
  void ScaleLandmarks(const float scale);
  //Comparisons
  bool operator==(const Shot& shot) const { return id_ == shot.id_; }
  bool operator!=(const Shot& shot) const { return !(*this == shot); }
  bool operator<(const Shot& shot) const { return id_ < shot.id_; }
  bool operator<=(const Shot& shot) const { return id_ <= shot.id_; }
  bool operator>(const Shot& shot) const { return id_ > shot.id_; }
  bool operator>=(const Shot& shot) const { return id_ >= shot.id_; }
  std::string GetCameraName() const { return shot_camera_.camera_name_; }
  const Camera& GetCameraModel() const { return shot_camera_.camera_model_; }
public:
  SLAMShotData slam_data_;
  //We could set the const values to public, to avoid writing a getter.
  const ShotId id_;
  const std::string name_;
  const ShotCamera& shot_camera_;
  ShotMeasurements shot_measurements_; //metadata

private:
  Pose pose_;
  size_t num_keypts_;
  std::vector<Landmark*> landmarks_;
  AlignedVector<Observation> keypoints_;
  DescriptorMatrix descriptors_;

  std::map<Landmark*, std::unique_ptr<Observation>>
      landmark_observations_;
};
}