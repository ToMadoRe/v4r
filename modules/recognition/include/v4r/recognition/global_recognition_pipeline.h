/******************************************************************************
 * Copyright (c) 2017 Thomas Faeulhammer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#pragma once

#include <v4r/recognition/global_recognizer.h>
#include <v4r/recognition/recognition_pipeline.h>
#include <v4r/segmentation/segmenter.h>
#include <omp.h>


namespace v4r
{

/**
 * @brief This class merges the results of various global descriptors into a set of hypotheses.
 * @author Thomas Faeulhammer
 * @date Jan 2017
 */
template<typename PointT>
class V4R_EXPORTS GlobalRecognitionPipeline : public RecognitionPipeline<PointT>
{
private:
    using RecognitionPipeline<PointT>::scene_;
    using RecognitionPipeline<PointT>::scene_normals_;
    using RecognitionPipeline<PointT>::obj_hypotheses_;
    using RecognitionPipeline<PointT>::m_db_;

    bool visualize_clusters_; ///< If set, visualizes the cluster and displays recognition information for each
    mutable pcl::visualization::PCLVisualizer::Ptr vis_;
    mutable int vp1_, vp2_, vp3_, vp4_, vp5_;
    mutable std::vector<std::string> coordinate_axis_ids_global_;

//    omp_lock_t rec_lock_;

    typename Segmenter<PointT>::Ptr seg_;
    std::vector<pcl::PointIndices> clusters_;

    std::vector<typename GlobalRecognizer<PointT>::Ptr > global_recognizers_; ///< set of Global recognizer generating keypoint correspondences
    std::vector<ObjectHypothesesGroup<PointT> > obj_hypotheses_wo_elongation_check_; ///< just for visualization (to see effect of elongation check)

    void visualize();

public:
    GlobalRecognitionPipeline ( )
    { }

    void initialize(const std::string &trained_dir, bool force_retrain = false);

    /**
     * @brief recognize
     */
    void
    recognize();

    /**
     * @brief addRecognizer
     * @param rec Global recognizer
     */
    void
    addRecognizer(const typename GlobalRecognizer<PointT>::Ptr & l_rec)
    {
        global_recognizers_.push_back( l_rec );
    }


    /**
     * @brief needNormals
     * @return
     */
    bool
    needNormals() const
    {
        for(size_t r_id=0; r_id < global_recognizers_.size(); r_id++)
        {
            if( global_recognizers_[r_id]->needNormals())
                return true;
        }

        return false;
    }

    void
    setSegmentationAlgorithm(const typename Segmenter<PointT>::Ptr &seg)
    {
        seg_ = seg;
    }

    /**
     * @brief getFeatureType
     * @return
     */
    size_t
    getFeatureType() const
    {
        size_t feat_type = 0;
        for(size_t r_id=0; r_id < global_recognizers_.size(); r_id++)
            feat_type += global_recognizers_[r_id]->getFeatureType();

        return feat_type;
    }

    bool
    requiresSegmentation() const
    {
        return true;
    }


    typedef boost::shared_ptr< GlobalRecognitionPipeline<PointT> > Ptr;
    typedef boost::shared_ptr< GlobalRecognitionPipeline<PointT> const> ConstPtr;
};

}