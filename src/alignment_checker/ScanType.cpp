#include "alignment_checker/ScanType.h"
namespace CorAlignment {

int PoseScan::pose_count = 1;

std::string Scan2str(const scan_type& val){
    switch (val){
    case rawlidar: return "rawlidar";
    case rawradar: return "rawradar";
    case kstrong: return "kstrong";
    case kstrongStructured: return "kstrongStructured";
    case cen2018: return "cen2018";
    case cen2019: return "cen2019";
    case cfear: return "cfear";
    case kstrongCart: return "kstrongCart";
    default: return "none";
    }
}
scan_type Str2Scan(const std::string& val){
    if (val=="rawlidar")
        return scan_type::rawlidar;
    else if (val=="rawradar")
        return scan_type::rawradar;
    else if (val=="kstrong")
        return scan_type::kstrong;
    else if (val=="kstrongStructured")
        return scan_type::kstrongStructured;
    else if (val=="cen2018")
        return scan_type::cen2018;
    else if (val=="cen2019")
        return scan_type::cen2019;
    else if (val=="kstrongCart")
        return scan_type::kstrongCart;
    else if (val=="none")
        return scan_type::none;
    else if (val=="cfear")
        return scan_type::cfear;
}
PoseScan::PoseScan(const PoseScan::Parameters pars, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : Test_(T), Tmotion_(Tmotion), cloud_(new pcl::PointCloud<pcl::PointXYZI>()), pose_id(pose_count++),pars_(pars)
{

}

RawRadar::RawRadar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : PoseScan(pars,T,Tmotion), range_res_(pars.range_res)
{
    polar_ = polar;
}

Cen2018Radar::Cen2018Radar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : RawRadar(pars, polar, T, Tmotion){
    //cout<<"cen 2018"<<endl;
    polar_->image.convertTo(f_polar_, CV_32F, 1/255.0);
    auto time = cen2018features(f_polar_, targets_);
    //cout<<"p: "<<targets_<<endl;
    pcl::PointXYZI p;
    const double nb_azimuths = f_polar_.rows;
    for(int i=0;i<targets_.cols();i++){
        const double azimuth_bin = targets_(0  ,i);
        const double range_bin = targets_(1  ,i);
        const double theta = (double(azimuth_bin + 1) / nb_azimuths) * 2. * M_PI;
        const double r = range_res_*range_bin;
        p.x = r*cos(theta);
        p.y = r*sin(theta);
        p.intensity =(float)polar_->image.at<uchar>(azimuth_bin,range_bin);
        cloud_->push_back(p);
    }
    radar_mapping::Compensate(cloud_, Tmotion_, pars.ccw); ;
}

Cen2019Radar::Cen2019Radar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : RawRadar(pars, polar, T, Tmotion){
/*
    cout<<"cen 2019"<<endl;
    polar_->image.convertTo(f_polar_, CV_32F, 1/255.0);
    auto time = cen2019features(f_polar_, targets_);
    cout<<"p: "<<targets_.rows()<<","<<targets_.cols()<<endl;


    cout<<"cen 2019 end"<<targets_.rows()<<endl;
*/
}


kstrongRadar::kstrongRadar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : RawRadar(pars, polar, T, Tmotion)
{
    assert(polar !=NULL);

    radar_mapping::k_strongest_filter(polar, cloud_, pars.kstrong, pars.z_min, pars.range_res, pars.sensor_min_distance);
    assert(cloud_ != nullptr);
    if(pars.compensate){
        radar_mapping::Compensate(cloud_, Tmotion_, pars.ccw); //cout<<"k strongest: "<<cloud_->size()<<endl;
    }
}
kstrongStructuredRadar::kstrongStructuredRadar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion)
    : RawRadar(pars, polar, T, Tmotion)
{
    assert(polar !=NULL);
    radar_mapping::StructuredKStrongest kstrong(polar_, pars.z_min, pars.kstrong, pars.sensor_min_distance, pars.range_res);
    kstrong.getPeaksFilteredPointCloud(cloud_,true); // get peaks
    //kstrong_peaks_ = cloud_;
    if(pars.compensate){
        radar_mapping::Compensate(cloud_, Tmotion_, pars.ccw); //cout<<"k strongest: "<<cloud_->size()<<endl;
    }
}

CFEARFeatures::CFEARFeatures(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion )
    : kstrongRadar(pars, polar, T, Tmotion)
{

    CFEARFeatures_ = radar_mapping::MapNormalPtr(new radar_mapping::MapPointNormal(cloud_, pars.resolution));
    //cout<<"frame: "<<pose_id<<"time: "<<cloud_->header.stamp<<", "<<cloud_->size()<<", "<<CFEARFeatures_->GetSize()<<endl;
}


CartesianRadar::CartesianRadar(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr& polar, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion )
    : RawRadar(pars,polar,T,Tmotion), sensor_min_distance(pars.sensor_min_distance), cart_resolution_(pars.cart_resolution), cart_pixel_width_(pars.cart_pixel_width){
    polar_ = polar;
    cart_ = boost::make_shared<cv_bridge::CvImage>();
    cart_->encoding = polar_->encoding;
    cart_->header.stamp = polar_->header.stamp;
    //cout<<"CartesianRadar::CartesianRadar"<<endl;
    radar_mapping::KstrongestPolar filter(pars.z_min, pars.kstrong, pars.sensor_min_distance);
    //filter.getFilteredImage(polar_,polar_filtered_);
    polar_->image.convertTo(polar_->image, CV_32F, 1/255.0);


    std::vector<double> azimuths;
    for (int bearing = 0; bearing < polar->image.rows; bearing++)
        azimuths.push_back( ((double)(bearing+1) / polar_->image.rows) * 2 * M_PI);

    radar_polar_to_cartesian(polar_->image, azimuths, cart_->image);

}

pcl::PointCloud<pcl::PointXYZI>::Ptr PoseScan::GetCloudCopy(const Eigen::Affine3d& T){
    pcl::PointCloud<pcl::PointXYZI>::Ptr transformed( new pcl::PointCloud<pcl::PointXYZI>());
    pcl::transformPointCloud(*cloud_, *transformed, T);
    return transformed;
}

PoseScan_S RadarPoseScanFactory(const PoseScan::Parameters& pars, cv_bridge::CvImagePtr radar_msg, const Eigen::Affine3d& T, const Eigen::Affine3d& Tmotion){
    if(pars.scan_type == rawradar)
        return PoseScan_S(new RawRadar(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == kstrong)
        return PoseScan_S(new kstrongRadar(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == ScanType::kstrongStructured)
        return PoseScan_S(new kstrongStructuredRadar(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == cfear)
        return PoseScan_S(new CFEARFeatures(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == kstrongCart)
        return PoseScan_S(new CartesianRadar(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == cen2018)
        return PoseScan_S(new Cen2018Radar(pars, radar_msg, T, Tmotion));
    else if(pars.scan_type == cen2019)
        return PoseScan_S(new Cen2019Radar(pars, radar_msg, T, Tmotion));

    else return nullptr;
}



}
