// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

/**
* @brief Class for detection with action info
*/
struct DetectedAction {
    /** @brief BBox of detection */
    cv::Rect rect;
    /** @brief Action label */
    int label;
    /** @brief Confidence of detection */
    float detection_conf;
    /** @brief Confidence of predicted action */
    float action_conf;

    /**
    * @brief Constructor
    */
    DetectedAction(const cv::Rect& rect, int label,
                   float detection_conf, float action_conf)
        : rect(rect), label(label), detection_conf(detection_conf),
          action_conf(action_conf) {}
};
using DetectedActions = std::vector<DetectedAction>;

/**
* @brief Class to store SSD-based head info
*/
struct SSDHead {
    /** @brief Step size for the head */
    int step;
    /** @brief Vector of anchors */
    std::vector<cv::Size2f> anchors;

    /**
    * @brief Constructor
    */
    SSDHead(int step, const std::vector<cv::Size2f>& anchors) : step(step), anchors(anchors) {}
};
using SSDHeads = std::vector<SSDHead>;

/**
* @brief Config for the Action Detection model
*/
struct ActionDetectorConfig {
    /** @brief Person detection action recognition 0006 network enable flag */
    bool new_network = false;
    /** @brief Scale paramter for Soft-NMS algorithm */
    float nms_sigma = 0.6f;
    /** @brief Threshold for detected objects */
    float detection_confidence_threshold = 0.4f;
    /** @brief Threshold for recognized actions */
    float action_confidence_threshold = 0.75f;
    /** @brief Scale of action logits for the old network version */
    float old_action_scale = 3.f;
    /** @brief Scale of action logits for the new network version */
    float new_action_scale = 16.f;
    /** @brief Default action class label */
    int default_action_id = 0;
    /** @brief Number of top-score bboxes in output */
    int keep_top_k = 200;
    /** @brief Number of SSD anchors for the old network version */
    std::vector<int> old_anchors{4};
    /** @brief Number of SSD anchors for the new network version */
    std::vector<int> new_anchors{1, 4};
    /** @brief Number of actions to detect */
    size_t num_action_classes = 3;
    /** @brief  SSD bbox encoding variances */
    float variances[4]{0.1f, 0.1f, 0.2f, 0.2f};
    SSDHeads new_det_heads{{8,  {{26.17863728f, 58.670372f}}},
                           {16, {{35.36f, 81.829632f},
                                 {45.8114572f, 107.651852f},
                                 {63.31491832f, 142.595732f},
                                 {93.5070856f, 201.107692f}}}};
};


class ActionDetection  {
public:
    explicit ActionDetection(const ActionDetectorConfig& config);

    DetectedActions fetchResults(const cv::Mat &in_ssd_local,
                                 const cv::Mat &in_ssd_conf,
                                 const cv::Mat &in_ssd_priorbox,
                                 const cv::Mat &in_ssd_anchor1,
                                 const cv::Mat &in_ssd_anchor2,
                                 const cv::Mat &in_ssd_anchor3,
                                 const cv::Mat &in_ssd_anchor4,
                                 const cv::Mat &in_frame);

private:
    ActionDetectorConfig config_;
    float width_ = 0;
    float height_ = 0;
    bool new_network_ = false;
    std::vector<int> head_ranges_;
    std::vector<int> head_step_sizes_;
    std::vector<cv::Size> head_blob_sizes_;
    std::vector<std::vector<int>> glob_anchor_map_;
    std::vector<std::string> glob_anchor_names_;
    int num_glob_anchors_;
    cv::Size network_input_size_;
    int num_candidates_;
    bool binary_task_;

    /**
    * @brief BBox in normalized form (each coordinate is in range [0;1]).
    */
    struct NormalizedBBox {
        float xmin;
        float ymin;
        float xmax;
        float ymax;
    };
    typedef std::vector<NormalizedBBox> NormalizedBBoxes;

     /**
    * @brief Translates the detections from the network outputs
    *
    * @param loc Location buffer
    * @param main_conf Detection conf buffer
    * @param add_conf Action conf buffer
    * @param priorboxes Priorboxes buffer
    * @param frame_size Size of input image (WxH)
    * @return Detected objects
    */
    DetectedActions GetDetections(const cv::Mat& loc,
                                  const cv::Mat& main_conf,
                                  const cv::Mat& priorboxes,
                                  const std::vector<cv::Mat>& add_conf,
                                  const cv::Size& frame_size) const;

     /**
    * @brief Translate input buffer to BBox
    *
    * @param data Input buffer
    * @return BBox
    */
    inline NormalizedBBox
    ParseBBoxRecord(const float* data, bool inverse) const;


     /**
    * @brief Translate input buffer to BBox
    *
    * @param data Input buffer
    * @return BBox
    */
    inline NormalizedBBox
    GeneratePriorBox(int pos, int step, const cv::Size2f& anchor, const cv::Size& blob_size) const;

     /**
    * @brief Translates input blobs in SSD format to bbox in CV_Rect
    *
    * @param prior_bbox Prior boxes in SSD format
    * @param variances Variances of prior boxes in SSD format
    * @param encoded_bbox BBox to decode
    * @param frame_size Size of input image (WxH)
    * @return BBox in CV_Rect format
    */
    cv::Rect ConvertToRect(const NormalizedBBox& prior_bbox,
                           const NormalizedBBox& variances,
                           const NormalizedBBox& encoded_bbox,
                           const cv::Size& frame_size) const;

     /**
    * @brief Carry out Soft Non-Maximum Suppression algorithm under detected actions
    *
    * @param detections Detected actions
    * @param sigma Scale paramter
    * @param top_k Number of top-score bboxes
    * @param min_det_conf Minimum detection confidence
    * @param out_indices Out indices of valid detections
    */
    void SoftNonMaxSuppression(const DetectedActions& detections,
                               const float sigma,
                               const int top_k,
                               const float min_det_conf,
                               std::vector<int>* out_indices) const;
};
