float _angleDribble(Vector2f vec){
	 return relativeBall().angle();
}
Vector2f orientation=Vector2f(1,0);

const float Pi = (float)3.14159265;
const float Inf=1e6;
const float ballRadius=50;

int numPatrolNow=0;
const int numPointPatrol=2;
Vector2f pointPatrol[2]={Vector2f(2500,1000),Vector2f(2500,-1000)};

Vector2f kickTarget=Vector2f(4550,0);

float target2Angle(Vector2f target){
	return (Transformation::fieldToRobot(theRobotPose,target)).angle();
}

Vector2f relativeBall(){
	return theBallModel.estimate.position;
}
Vector2f fieldBall(){
	return Transformation::robotToField(theRobotPose,relativeBall());
}

float timeSinceBallWasSeen(){
	return theFrameInfo.getTimeSince(theTeamBallModel.timeWhenLastSeen);
}

void printSegment(Vector2f a,Vector2f b,std::string color,std::ofstream& out){
	out<<"segment"<<std::endl<<color<<std::endl<<a<<std::endl<<b<<std::endl;
}
void printPoint(Vector2f point,std::string note=""){
	std::ofstream out;
	static int countPrint=0;//累计输出次数
	if(countPrint%30==0)
		out.open("/home/dingjiayu/Bhuman/data.txt",std::ios::app);
	else
		out.open("/home/dingjiayu/Bhuman/dataNotUsed.txt");
	++countPrint;
	out<<note<<std::endl
	   <<Global::getSettings().playerNumber<<std::endl
	   <<point<<std::endl;
	out.close();
}

Vector2f maxElement(const std::vector<std::pair<float,Vector2f> >& vec){
	float max=vec[0].first;
	Vector2f maxEle=vec[0].second;
	for(const auto& i:vec){
		if(i.first>max){
			max=i.first;
			maxEle=i.second;
		}
	}
	return maxEle;
}

Vector2f minElement(const std::vector<std::pair<float,Vector2f> >& vec){
	float min=vec[0].first;
	Vector2f minEle=vec[0].second;
	for(const auto& i:vec){
		if(i.first<min){
			min=i.first;
			minEle=i.second;
		}
	}
	return minEle;
}

//线段
class segment{
	public:
	Vector2f end[2];
	
	segment(){
		end[0]=end[1]=Vector2f(0,0);
	}
	segment(Vector2f a,Vector2f b){
		end[0]=a;
		end[1]=b;
	}
};
//veca与vecb的行列式
float det(Vector2f veca, Vector2f vecb){
	return veca[0]*vecb[1]-veca[1]*vecb[0];
}
float min(float numa,float numb){
	return numa<numb?numa:numb;
}
float max(float numa,float numb){
	return numa>numb?numa:numb;
}
//两线段是否相交
bool ifTwoSegmentIntersect(segment sega,segment segb){
	//快速排斥
	if(min(sega.end[0][0],sega.end[1][0])>max(segb.end[0][0],segb.end[1][0]) ||
	   min(segb.end[0][0],segb.end[1][0])>max(sega.end[0][0],sega.end[1][0]) ||
	   min(sega.end[0][1],sega.end[1][1])>max(segb.end[0][1],segb.end[1][1]) ||
	   min(segb.end[0][1],segb.end[1][1])>max(sega.end[0][1],sega.end[1][1])
	)
		return false;
	//跨立检验
	Vector2f a0b0=segb.end[0]-sega.end[0],a0b1=segb.end[1]-sega.end[0],a0a1=sega.end[1]-sega.end[0];
	if(det(a0b0,a0a1)*det(a0b1,a0a1)>1e-6)
		return false;
	Vector2f b0a0=sega.end[0]-segb.end[0],b0a1=sega.end[1]-segb.end[0],b0b1=segb.end[1]-segb.end[0];
	if(det(b0a0,b0b1)*det(b0a1,b0b1)>1e-6)
		return false;
	return true;
}
//pointa与pointb距离的平方
float disSquare(Vector2f pointa,Vector2f pointb){
	return (pointa[0]-pointb[0])*(pointa[0]-pointb[0])+(pointa[1]-pointb[1])*(pointa[1]-pointb[1]);
}
//点与线段的最短距离
float minDisBetweenPointAndSegment(Vector2f point,segment seg){
	//设线段端点为a，b，另一点为c
	Vector2f ab=seg.end[1]-seg.end[0],ac=point-seg.end[0];
	float lenseg=disSquare(seg.end[0],seg.end[1]),innerproduct=ab[0]*ac[0]+ab[1]*ac[1];//内积
	if(innerproduct<0)//垂足在a的外侧
		return std::sqrt(disSquare(point,seg.end[0]));
	else if(innerproduct>lenseg)//垂足在b的外侧
		return std::sqrt(disSquare(point,seg.end[1]));
	else{//垂足在ab上
		return std::sqrt(disSquare(point,seg.end[0])-innerproduct*innerproduct/lenseg);
	}
}
//两线段之间最短距离
float minDisBetweenTwoSegment(segment sega,segment segb){
	if(ifTwoSegmentIntersect(sega,segb)){
		return 0;
	}
	else{
		//四端点离另一线段的最短距离
		float dis[4],min;
		dis[0]=minDisBetweenPointAndSegment(sega.end[0],segb);
		dis[1]=minDisBetweenPointAndSegment(sega.end[1],segb);
		dis[2]=minDisBetweenPointAndSegment(segb.end[0],sega);
		dis[3]=minDisBetweenPointAndSegment(segb.end[1],sega);
		
		//四个距离中的最小值
		min=dis[0];
		for(int i=1;i<=3;++i){
			if(dis[i]<min){
				min=dis[i];
			}
		}
		return min;
	}
}

float minDisBetweenSegmentAndObstacle(segment seg){
	float disTemp, minDis=Inf;
	Vector2f posObstacle;
	segment segObstacle;
	for(const auto& obstacle : theObstacleModel.obstacles){
		posObstacle=Transformation::robotToField(theRobotPose,obstacle.center);
		if((posObstacle-theRobotPose.translation).norm()>1e-6 &&//除去自己
			std::fabs(posObstacle[0])<=theFieldDimensions.xPosOpponentGoal &&//除去边界外障碍
			std::fabs(posObstacle[1])<=theFieldDimensions.yPosLeftSideline)
		{
			  segObstacle.end[0]=Transformation::robotToField(theRobotPose,obstacle.left);
			  segObstacle.end[1]=Transformation::robotToField(theRobotPose,obstacle.right);
			  disTemp=minDisBetweenTwoSegment(segObstacle,seg);
			  if(disTemp<minDis)
				  minDis=disTemp;
		}
	}
	
    segObstacle.end[0]=Vector2f(4500,800);
	segObstacle.end[1]=Vector2f(5200,800);
	disTemp=minDisBetweenTwoSegment(segObstacle,seg);
    if(disTemp<minDis)
	   minDis=disTemp;
	
	segObstacle.end[0]=Vector2f(4500,-800);
	segObstacle.end[1]=Vector2f(5200,-800);
	disTemp=minDisBetweenTwoSegment(segObstacle,seg);
    if(disTemp<minDis)
	   minDis=disTemp;
	
	return minDis;
}

//是否不会被障碍物阻挡或蹭到
bool ifKickAvoidObstacle(Vector2f targetPoint){
	segment seg(theTeamBallModel.position,targetPoint);
	return minDisBetweenSegmentAndObstacle(seg)>ballRadius;
}

float getMinDisToOppo(Vector2f relativePos){
	float minDis=Inf,oneDis;
	for(const auto& obstacle : theObstacleModel.obstacles){
		if(obstacle.type==Obstacle::opponent || obstacle.type==Obstacle::someRobot){
			oneDis=(obstacle.center-relativePos).norm();
			if(oneDis<minDis)
				minDis=oneDis;
		}
	}
	return minDis;
}

bool getShootTarget(Vector2f& shootTarget){
	const float step=2*theFieldDimensions.yPosLeftGoal/400;
	const float shootLen=4000;
	
	Vector2f pointGoal(theFieldDimensions.xPosOpponentGroundline+ballRadius,0);
	segment segFromBallToGoal(fieldBall(),pointGoal);
	std::vector<std::pair<float,Vector2f> > candidates;
	std::pair<float,Vector2f> temp;
    
    if((fieldBall()-shootTarget).norm()<shootLen){
        segFromBallToGoal.end[1]=shootTarget;
        if(minDisBetweenSegmentAndObstacle(segFromBallToGoal)>ballRadius){
            return true;
        }
    }
    
    
	for(float i=theFieldDimensions.yPosRightGoal;i<=theFieldDimensions.yPosLeftGoal;i+=step){
		pointGoal[1]=i;
		if((fieldBall()-pointGoal).norm()<shootLen){
			segFromBallToGoal.end[1]=pointGoal;
			temp.first=minDisBetweenSegmentAndObstacle(segFromBallToGoal);
			if(temp.first>ballRadius){
				temp.second=pointGoal;
				candidates.push_back(temp);
			}
		}
	}
	
	if(!candidates.empty())
		shootTarget=maxElement(candidates);
	
	return !candidates.empty();
}

bool getSwiftShootTarget(Vector2f& shootTarget){
	const float step=2*theFieldDimensions.yPosLeftGoal/400;
	const float shootLen=4000;
	
	Vector2f pointGoal(theFieldDimensions.xPosOpponentGroundline+ballRadius,0);
	segment segFromBallToGoal(fieldBall(),pointGoal);
	std::vector<std::pair<float,Vector2f> > candidates;
	std::pair<float,Vector2f> temp;
    
    if((fieldBall()-shootTarget).norm()<shootLen){
        segFromBallToGoal.end[1]=shootTarget;
        if(minDisBetweenSegmentAndObstacle(segFromBallToGoal)>ballRadius){
            return true;
        }
    }
    
	for(float i=theFieldDimensions.yPosRightGoal;i<=theFieldDimensions.yPosLeftGoal;i+=step){
		pointGoal[1]=i;
		if((fieldBall()-pointGoal).norm()<shootLen){
			segFromBallToGoal.end[1]=pointGoal;
            if(minDisBetweenSegmentAndObstacle(segFromBallToGoal)>ballRadius){
                Vector2f v1,v2;
                v1 = fieldBall()-pointGoal;
                v2 = theRobotPose.translation-fieldBall();
                temp.first = (v1[0]*v2[0]+v1[1]*v2[1])/(v1.norm()*v2.norm());
                temp.second = pointGoal;
                candidates.push_back(temp);
            }
		}
	}
    
	if(!candidates.empty())
		shootTarget=minElement(candidates);
	
	return !candidates.empty();
}

bool ifClosestToBall(){
	float myDisToBall=(theRobotPose.translation-theTeamBallModel.position).norm();
	float otherDisToBall;
	Vector2f otherPos;
	bool ifCloest=true;
	for(const auto& teammate : theTeamData.teammates){
		otherPos=teammate.theRobotPose.translation;
		if((otherPos-theRobotPose.translation).norm()>1e-3){
			otherDisToBall=(otherPos-theTeamBallModel.position).norm();
			if(otherDisToBall<myDisToBall){
				ifCloest=false;
				break;
			}
		}
	}
	return ifCloest;
}