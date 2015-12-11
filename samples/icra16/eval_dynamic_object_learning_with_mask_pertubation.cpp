
#include <v4r/common/miscellaneous.h>
#include <v4r/object_modelling/do_learning.h>
#include <v4r/io/filesystem.h>
#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <iostream>
#include <fstream>
#include <time.h>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

//-radius 0.01 -dot_product 0.95 -normal_method 1 -chop_z 1.8  -scenes_dir  /media/Data/datasets/TUW/test_set_icra16 -input_mask_dir /media/Data/datasets/TUW/TUW_gt_models -output_dir /home/thomas/Documents/TUW_dol_eval/ -visualize 0 -min_plane_points 100 -ratio_cluster_occluded 0.75 -ratio_cluster_obj_supported 0.25 -inlier_threshold_plane_seg 0.05

//-do_erosion 1 -radius 0.005 -dot_product 0.99 -normal_method 0 -chop_z 2 -transfer_latest_only 0 -do_sift_based_camera_pose_estimation 0 -scenes_dir /media/Data/datasets/TUW/test_set -input_mask_dir /home/thomas/Desktop/test -output_dir /home/thomas/Desktop/out_test/ -visualize 1

int
main (int argc, char ** argv)
{
    typedef pcl::PointXYZRGB PointT;

    std::string scene_dir, input_mask_dir, output_dir = "/tmp/dol/", output_rec_dir="/tmp/dol_rec/",
            recognition_structure_dir = "/tmp/recognition_structure_dir";

    bool visualize = false;
    size_t min_mask_points = 50;

    v4r::object_modelling::DOL m;

    po::options_description desc("Evaluation Dynamic Object Learning with Ground Truth\n======================================\n **Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("scenes_dir,s", po::value<std::string>(&scene_dir)->required(), "input directory with .pcd files of the scenes. Each folder is considered as seperate sequence. Views are sorted alphabetically and object mask is applied on first view.")
            ("input_mask_dir,m", po::value<std::string>(&input_mask_dir)->required(), "directory containing the object masks used as a seed to learn the object in the first cloud")
            ("output_dir,o", po::value<std::string>(&output_dir)->default_value(output_dir), "Output directory where the model and parameter values will be stored")
            ("output_rec_dir", po::value<std::string>(&output_rec_dir)->default_value(output_rec_dir), "Output directory where the learnt model will be saved as recognition model (3D model + training views)")
            //("recognition_structure_dir", po::value<std::string>(&recognition_structure_dir)->default_value(recognition_structure_dir), "")

            ("radius,r", po::value<double>(&m.param_.radius_)->default_value(m.param_.radius_), "Radius used for region growing. Neighboring points within this distance are candidates for clustering it to the object model.")
            ("dot_product", po::value<double>(&m.param_.eps_angle_)->default_value(m.param_.eps_angle_), "Threshold for the normals dot product used for region growing. Neighboring points with a surface normal within this threshold are candidates for clustering it to the object model.")
            ("dist_threshold_growing", po::value<double>(&m.param_.dist_threshold_growing_)->default_value(m.param_.dist_threshold_growing_), "")
            ("seed_res", po::value<double>(&m.param_.seed_resolution_)->default_value(m.param_.seed_resolution_), "")
            ("voxel_res", po::value<double>(&m.param_.voxel_resolution_)->default_value(m.param_.voxel_resolution_), "")
            ("ratio", po::value<double>(&m.param_.ratio_supervoxel_)->default_value(m.param_.ratio_supervoxel_), "")
            ("do_erosion", po::value<bool>(&m.param_.do_erosion_)->default_value(m.param_.do_erosion_), "")
            ("do_mst_refinement", po::value<bool>(&m.param_.do_mst_refinement_)->default_value(m.param_.do_mst_refinement_), "")
            ("do_sift_based_camera_pose_estimation", po::value<bool>(&m.param_.do_sift_based_camera_pose_estimation_)->default_value(m.param_.do_sift_based_camera_pose_estimation_), "")
            ("transfer_latest_only", po::value<bool>(&m.param_.transfer_indices_from_latest_frame_only_)->default_value(m.param_.transfer_indices_from_latest_frame_only_), "")
            ("chop_z,z", po::value<double>(&m.param_.chop_z_)->default_value(m.param_.chop_z_), "Cut-off distance of the input clouds with respect to the camera. Points further away than this distance will be ignored.")
            ("normal_method,n", po::value<int>(&m.param_.normal_method_)->default_value(m.param_.normal_method_), "")
            ("ratio_cluster_obj_supported", po::value<double>(&m.param_.ratio_cluster_obj_supported_)->default_value(m.param_.ratio_cluster_obj_supported_), "")
            ("ratio_cluster_occluded", po::value<double>(&m.param_.ratio_cluster_occluded_)->default_value(m.param_.ratio_cluster_occluded_), "")

            ("stat_outlier_removal_meanK", po::value<int>(&m.sor_params_.meanK_)->default_value(m.sor_params_.meanK_), "MeanK used for statistical outlier removal (see PCL documentation)")
            ("stat_outlier_removal_std_mul", po::value<double>(&m.sor_params_.std_mul_)->default_value(m.sor_params_.std_mul_), "Standard Deviation multiplier used for statistical outlier removal (see PCL documentation)")
            ("inlier_threshold_plane_seg", po::value<double>(&m.p_param_.inlDist)->default_value(m.p_param_.inlDist), "")
            ("min_points_smooth_cluster", po::value<int>(&m.p_param_.minPointsSmooth)->default_value(m.p_param_.minPointsSmooth), "Minimum number of points for a cluster")
            ("min_plane_points", po::value<int>(&m.p_param_.minPoints)->default_value(m.p_param_.minPoints), "Minimum number of points for a cluster to be a candidate for a plane")
            ("smooth_clustering", po::value<bool>(&m.p_param_.smooth_clustering)->default_value(m.p_param_.smooth_clustering), "If true, does smooth clustering. Otherwise only plane clustering.")

            ("visualize,v", po::bool_switch(&visualize), "turn visualization on")
     ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    try
    {
        po::notify(vm);
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl << std::endl << desc << std::endl;
        return false;
    }

    m.initSIFT();

    std::stringstream output_rec_model, output_rec_structure;
    output_rec_model << output_rec_dir << "/models/";
    output_rec_structure << output_rec_dir << "/recognition_structure/";

    v4r::io::createDirIfNotExist(output_dir);
    v4r::io::createDirIfNotExist(output_rec_model.str());
    v4r::io::createDirIfNotExist(output_rec_structure.str());

    ofstream param_file;
    param_file.open ((output_dir + "/param.nfo").c_str());
    m.printParams(param_file);
    param_file    <<  "stat_outlier_removal_meanK" << m.sor_params_.meanK_ << std::endl
                  << "stat_outlier_removal_std_mul" << m.sor_params_.std_mul_ << std::endl
                  << "inlier_threshold_plane_seg" << m.p_param_.inlDist << std::endl
                  << "min_points_smooth_cluster" << m.p_param_.minPointsSmooth << std::endl
                  << "min_plane_points" << m.p_param_.minPoints << std::endl;
    param_file.close();

    srand(time(NULL));

    std::vector< std::string> sub_folder_names;
    if(!v4r::io::getFoldersInDirectory( scene_dir, "", sub_folder_names) )
    {
        std::cerr << "No subfolders in directory " << scene_dir << ". " << std::endl;
        sub_folder_names.push_back("");
    }

    std::sort(sub_folder_names.begin(), sub_folder_names.end());

    for (size_t sub_folder_id=0; sub_folder_id < sub_folder_names.size(); sub_folder_id++)
    {
        const std::string output_dir_w_sub = output_dir + "/" + sub_folder_names[ sub_folder_id ];
        v4r::io::createDirIfNotExist(output_dir_w_sub);

        const std::string annotations_dir = input_mask_dir + "/" + sub_folder_names[ sub_folder_id ];
        std::vector< std::string > mask_file_v;
        v4r::io::getFilesInDirectory(annotations_dir, mask_file_v, "", ".*.txt", false);

        std::sort(mask_file_v.begin(), mask_file_v.end());

        for (size_t o_id=0; o_id<mask_file_v.size(); o_id++)
        {
            const std::string mask_file = annotations_dir + "/" + mask_file_v[o_id];

            std::ifstream initial_mask_file;
            initial_mask_file.open( mask_file.c_str() );

            size_t idx_tmp;
            std::vector<size_t> mask;
            while (initial_mask_file >> idx_tmp)
            {
                mask.push_back(idx_tmp);
            }
            initial_mask_file.close();

            if ( mask.size() < min_mask_points) // not enough points to grow an object
                continue;

            cv::Mat mask_img = cv::Mat(480, 640, CV_8UC1);
            for(int r=0; r < mask_img.rows; r++)
                for(int c=0; c < mask_img.cols; c++)
                    mask_img.at<unsigned char>(r,c) = 0;

            for(size_t i=0; i < mask.size(); i++)
            {
                int r,c;
                r = mask[i] / mask_img.cols;
                c = mask[i] % mask_img.cols;

                mask_img.at<unsigned char>(r,c) = 255;
            }
            cv::Mat const structure_elem = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
            cv::Mat close_result;
            cv::morphologyEx(mask_img, close_result, cv::MORPH_CLOSE, structure_elem);

            cv::Mat mask_img_out = cv::Mat(480, 640, CV_8UC1);

            for (size_t noise_width = 5; noise_width < 81; noise_width=noise_width+10)
            {
//                cv::Mat mask_dst;/*
//                cv::erode(close_result, mask_dst, cv::Mat(), cv::Point(-1,-1), 1+std::floor(noise_width/4));
//                cv::blur( close_result, mask_img_out, cv::Size(noise_width, noise_width));
                cv::GaussianBlur(close_result, mask_img_out, cv::Size(noise_width, noise_width), 0, 0, cv::BORDER_DEFAULT );
//                cv::medianBlur( close_result, mask_img_out, noise_width);
//                cv::Mat mask_img = cv::Mat(480, 640, CV_8UC1);
//                for(size_t r=0; r < mask_img_out.rows; r++)
//                {
//                    for(size_t c=0; c < mask_img_out.cols; c++)
//                    {
//                        int tmp = std::abs( mask_img_out.at<unsigned char>(r,c) - (int)128);
//                        mask_img_out.at<unsigned char>(r,c) = tmp;
//                    }
//                 }
//                cv::namedWindow("mask");
//                cv::imshow("mask",mask_img);
//                cv::namedWindow("close");
//                cv::imshow("close",close_result);
//                cv::namedWindow("mask_img_out");
//                cv::imshow("mask_img_out",mask_img_out);
//                cv::namedWindow("diff");
                cv::Mat diff = cv::abs(mask_img_out-close_result);
//                std::cout << diff << std::endl;
//                cv::imshow("diff", diff);

                cv::Mat noise = cv::Mat(480, 640, CV_8UC1);
                for(int r=0; r < mask_img.rows; r++)
                {
                    for(int c=0; c < mask_img.cols; c++)
                    {
                        if (diff.at<unsigned char>(r,c) > 5)
                        {
                            if (rand()%255 > 128)
                                noise.at<unsigned char>(r,c)  = 255;
                            else
                                noise.at<unsigned char>(r,c)  = 0;
                        }
                        else
                            noise.at<unsigned char>(r,c) = mask_img.at<unsigned char>(r,c);
                    }
                }

             std::ofstream f;
             std::string out_mask_fn = output_dir_w_sub + "/" + mask_file_v[o_id];
             std::stringstream suffix_mask;
             suffix_mask << "mask_" << noise_width << ".txt";
             boost::replace_last (out_mask_fn, "mask.txt", suffix_mask.str());
             f.open(out_mask_fn.c_str());
             std::vector<size_t> mask_pertubated;
             for(int r=0; r < mask_img.rows; r++)
             {
                 for(int c=0; c < mask_img.cols; c++)
                 {
                     if (noise.at<unsigned char>(r,c)  > 128)
                     {
                         const int idx = r*640+c;
                         mask_pertubated.push_back(idx);
                         f << idx << std::endl;
                     }
                 }
             }
             f.close();
//                cv::namedWindow("noise");
//                cv::imshow("noise", cv::abs(noise));
//                cv::waitKey(0);
//                continue;

            const std::string scene_path = scene_dir + "/" + sub_folder_names[ sub_folder_id ];
            std::vector< std::string > views;
            v4r::io::getFilesInDirectory(scene_path, views, "", ".*.pcd", false);

            std::sort(views.begin(), views.end());

            std::cout << "Learning object from mask " << mask_file << " for scene " << scene_path << std::endl;

            for(size_t v_id=0; v_id<views.size(); v_id++)
            {
                const std::string view_file = scene_path + "/" + views[ v_id ];
                pcl::PointCloud<PointT>::Ptr pCloud(new pcl::PointCloud<PointT>());
                pcl::io::loadPCDFile(view_file, *pCloud);
                const Eigen::Matrix4f trans = v4r::RotTrans2Mat4f(pCloud->sensor_orientation_, pCloud->sensor_origin_);


                Eigen::Vector4f zero_origin;
                zero_origin[0] = zero_origin[1] = zero_origin[2] = zero_origin[3] = 0.f;
                pCloud->sensor_origin_ = zero_origin;   // for correct visualization
                pCloud->sensor_orientation_ = Eigen::Quaternionf::Identity();

                if (v_id==0)
                    m.learn_object(*pCloud, trans, mask_pertubated);
                else
                    m.learn_object(*pCloud, trans);
            }

            std::string out_fn = mask_file_v[o_id];
            std::stringstream suffix;
            suffix << "dol_" << noise_width << ".pcd";
            boost::replace_last (out_fn, "mask.txt", suffix.str());
            m.save_model(output_dir_w_sub, recognition_structure_dir, out_fn);
            m.write_model_to_disk(output_rec_model.str(), output_rec_structure.str(), sub_folder_names[ sub_folder_id ]+"_object.pcd");
            if (visualize)
                m.visualize();
            m.clear();
            }
        }
    }
    return 0;
}
