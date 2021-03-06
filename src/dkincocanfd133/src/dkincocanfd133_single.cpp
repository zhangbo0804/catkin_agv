
#include <string>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp> //for ref_list_of  
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/progress.hpp>
#include <boost/asio.hpp>



#include "../../zmq_ros/include/zmqclient.h"
#include "socketcan.hpp"
#include "dkincocanfd133Mode.hpp"

#include "ros/ros.h"
#include "ros/console.h"
#include "std_msgs/String.h"
#include "geometry_msgs/Twist.h"
#include "dgvmsg/Encounter.h"
#include "dgvmsg/EncounterV.h"


#define DEV_DRIVER_ADD_R  1
#define DEV_DRIVER_ADD_L  2


#define BROADCAST       0x0000
#define P2P             0x0100

//D9-D10
#define CHANGSTATUS     0x0480
#define CHANGCMD        0x0000
#define SUPERSTATUS     0x0000
#define SUPERCMD        0x0000

#define VER "QTsingtooRobot_v1-2016-06-22-1922"
#define NODENAME "dKincocanfd133Node"



int main(int argc, char **argv)
{
    ros::init(argc, argv, NODENAME);
	boost::shared_ptr< ros::NodeHandle> handel(new ros::NodeHandle);

	//设置接收过滤
    int fid[DRIVER_AMOUNT]={DEV_DRIVER_ADD_R+CHANGSTATUS+P2P,      // 0x0581
                            DEV_DRIVER_ADD_L+CHANGSTATUS+P2P};     // 0x0582

	//参数打印	
	std::cout<<"argc= "<<argc<<std::endl;	
    for(int index=0;index<argc;index++)
    {
        std::cout<<"argv"<<"["<<index<<"]= "<<argv[index]<<std::endl;
	}	

    if(argc != 6)
    {
        std::cout<<"argv: com add1 dir1 add2 dir2"<<std::endl;
        return 0;
    }

    // use argument vector from argv
    std::string canport_id=CAN1;
    std::string argv_add[DRIVER_AMOUNT];        //
    std::string argv_dir[DRIVER_AMOUNT];        // wheel direction
	
    canport_id = argv[1];
	argv_add[0] = argv[2];
    argv_dir[0] = argv[3];

    argv_add[1] = argv[4];
    argv_dir[1] = argv[5];

    if( (canport_id != CAN1) && (canport_id != CAN2) )
    {
        std::cout<<"argv[1]: canport_id  wrong ! "<<std::endl;
        return 0;
    }

    int dev_add[DRIVER_AMOUNT] = {127,127};
    for(int index=0; index<DRIVER_AMOUNT; index++)
    {
        try
        {
            dev_add[index]=boost::lexical_cast<int>(argv_add[index]); // 将字符串转化为整数
            if((dev_add[index] <=0)&&(dev_add[index] >127))
            {
                std::cout<<"add wrong ! "<<(int)dev_add[index]<<std::endl;
                return 0;
            }
        }
        catch(boost::bad_lexical_cast&)    // 转换失败会抛出一个异常
        {
            dev_add[index]=0;
            std::cout<<"add wrong ! "<<std::endl;
            return 0;
        }
    }

	int dir[DRIVER_AMOUNT]={1,1};
    for(int index=0; index<DRIVER_AMOUNT; index++)
    {
        try
        {
            dir[index] = boost::lexical_cast<int>(argv_dir[index]); // 将字符串转化为整数
            if((dir[index] != 1 ) && (dir[index] != -1 ))
            {
                std::cout<<"dir wrong ! "<<std::endl;
                return 0;
            }
        }
        catch(boost::bad_lexical_cast&)    // 转换失败会抛出一个异常
        {
            dir[index]=1;
            std::cout<<"dir wrong ! "<<std::endl;
            return 0;
        }
    }


    // commented by ssj 4-10
    // int canrecv_driver_id = (int)(CHANGSTATUS+P2P+kincocanfd133Mode::MYADD);
    boost::shared_ptr<socketcan> drivcan(new socketcan(canport_id, fid, sizeof(fid)/sizeof(int)) );
	 
    std::vector < boost::shared_ptr<socketcan> > veccanpa;
    veccanpa.reserve(2);
    for(int size=0; size <2; size++)
    {
        veccanpa.push_back(drivcan);        // why??? veccanpa[1] never be used.
    }

    kincocanfd133Mode Kdriver(canport_id, dev_add, dir, DRIVER_AMOUNT);
	
	// pub 
	Kdriver.rospub_mode_driverstatues = handel->advertise<std_msgs::String>("Kincodriverfeedback", 10);
	//ROS_INFO("  Initialization rossub_mode_driver Kincodriverfeedback port  is ok");
	Kdriver.rospub_mode_encounter = handel->advertise<dgvmsg::Encounter>("driverencounter", 10);

	//SUB
	ros::Subscriber rossub_mode_driver =
                        handel->subscribe("Net2Ctrl", 10,  &kincocanfd133Mode::Net2Ctrl_driverCall,&Kdriver);
	//ROS_INFO("  Initialization rossub_mode_driver Net2Ctrl port  is ok");

	ros::Subscriber rossub_mode_Sdriver = 
                        handel->subscribe("Net2SCtrl", 10, &kincocanfd133Mode::Net2SCtrl_driverCall,&Kdriver);
	//ROS_INFO("  Initialization rossub_mode_Sdriver Net2SCtrl  port  is ok");

	
    boost::thread t1(boost::bind(&kincocanfd133Mode::kincocanfd133Mode_master, &Kdriver, veccanpa));
	
	ros::spin();
	t1.join();

	return 0;

}



