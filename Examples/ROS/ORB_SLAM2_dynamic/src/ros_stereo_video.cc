/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <opencv2/core/core.hpp>
#include"/home/lnss/VORBSLAM/include/orbslam/System.h"
#include<init.hpp>
unsigned int *hfids,*hchild;
float *hthrs,*hhs;
float *hp[19],*hsp[19],*hpadp[19];
int  hh[19] = {160, 147, 135, 124, 113, 104, 95, 87, 80, 73, 67, 61, 56, 52, 48, 44, 40, 37, 33};
int  wh[19] = {120, 110, 101, 93,  85,  78,  71, 65, 60, 55, 50, 46, 42, 39, 36, 33, 30, 28, 25};
unsigned long int i=0;
double dtime=0;
using namespace std;
int main(int argc, char **argv)
{
    ros::init(argc, argv, "RGBD");
    ros::start();
	Init();
    if(argc != 3)
    {
        cerr << endl << "Usage: rosrun ORB_SLAM2 Stereo path_to_vocabulary path_to_settings" << endl;
        ros::shutdown();
        return 1;
    }    
    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::STEREO,true);
    ros::NodeHandle nh;
    cv::VideoCapture captureleftvideo;
	cv::VideoCapture capturerightvideo;
	captureleftvideo.open("/home/lnss/video_left.avi");
	capturerightvideo.open("/home/lnss/video_right.avi");		
	char depthpath[40];
	if((!captureleftvideo.isOpened())||(!captureleftvideo.isOpened()))
	{
		cout<<"fail to load video"<<endl;
		return 0;
	}
	cv::Mat KLeft(3,3,CV_64F);
	KLeft.at<double>(0,0)=557.3402395832574;KLeft.at<double>(0,1)=0;KLeft.at<double>(0,2)=520.640108095284;
	KLeft.at<double>(1,0)=0;KLeft.at<double>(1,1)=556.6557557023757;KLeft.at<double>(1,2)=507.53704440371985;
	KLeft.at<double>(2,0)=0;KLeft.at<double>(2,1)=0;KLeft.at<double>(2,2)=1;
	
	cv::Mat KRight(3,3,CV_64F);
	KRight.at<double>(0,0)=557.0733629673801;KRight.at<double>(0,1)=0;KRight.at<double>(0,2)=518.6028160033426;
	KRight.at<double>(1,0)=0;KRight.at<double>(1,1)=556.4361577915784;KRight.at<double>(1,2)=506.4748363386079;
	KRight.at<double>(2,0)=0;KRight.at<double>(2,1)=0;KRight.at<double>(2,2)=1;
	
	cv::Mat R(3,3,CV_64F);
	R.at<double>(0,0)=0.99999998;R.at<double>(0,1)=-0.00008;R.at<double>(0,2)=-0.00020418;
	R.at<double>(1,0)=0.00008028;R.at<double>(1,1)=0.999999;R.at<double>(1,2)=0.001558;
	R.at<double>(2,0)=0.0002041;R.at<double>(2,1)=-0.0015580772636672976;R.at<double>(2,2)=0.999999;
	
	cv::Mat T(3,1,CV_64F);
	T.at<double>(0,0)=-0.06985;T.at<double>(1,0)=-0.00001756;T.at<double>(2,0)=0.00020;
	
	cv::Mat R1;
	cv::Mat P1;
	cv::Mat R2;
	cv::Mat P2;
	cv::Mat Q;
	cv::Mat rmap[2][2];
	 
    cv::stereoRectify(KLeft,Mat::zeros(1,4,CV_64F),KRight,Mat::zeros(1,4,CV_64F),
        cv::Size(1024,1024),R,T,R1,R2,P1,P2,Q);

	while(ros::ok())
	{
		cv::Mat left,right,depth,lefti,righti;
		sprintf(depthpath,"%s%lu%s","/home/lnss/depth/",i,".png");
		captureleftvideo.read(left);
		capturerightvideo.read(right);
		depth=cv::imread(depthpath,CV_LOAD_IMAGE_ANYDEPTH);
		if(left.empty()||right.empty()||depth.empty())
		{	
			if(i!=0)
				cout<<"all frames are processed"<<endl;
			i=0;			
			continue;		
		}
		
		initUndistortRectifyMap(KLeft,Mat::zeros(1,5,CV_64F), R1, P1, cv::Size(1024,1024), CV_32F,rmap[0][0], rmap[0][1]);
		    
    	initUndistortRectifyMap(KRight, Mat::zeros(1,5,CV_64F), R2, P2, cv::Size(1024,1024), CV_32F,rmap[1][0], rmap[1][1]);
		cv::remap(left,lefti,rmap[0][0],rmap[0][1],cv::INTER_LINEAR);
        cv::remap(right,righti,rmap[1][0],rmap[1][1],cv::INTER_LINEAR);
		//cv::imwrite("3.jpg",left);
		//cv::imwrite("4.jpg",right);
		cv::waitKey(1);
          	SLAM.TrackStereo(lefti,righti,depth,dtime);
		i++;	
	}
    // Stop all threads
    SLAM.Shutdown();
    // Save camera trajectory
    //SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory_TUM_Format.txt");
    //SLAM.SaveTrajectoryTUM("FrameTrajectory_TUM_Format.txt");
    SLAM.SaveTrajectoryKITTI("FrameTrajectory_KITTI_Format.txt");
    ros::shutdown();
    return 0;
}




