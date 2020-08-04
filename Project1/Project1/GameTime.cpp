

#include "GameTime.h"

GameTime::GameTime():mSencondsPerCount(0.0),mDeltaTime(-1.0),mBaseTime(0),
mPauseTime(0),mStopTime(0),mPrevTime(0),mCurrentTime(0),isStoped(false)
{
	//计算计数器每秒多少次，并存入countsPerSec中返回
	//注意，此处为QueryPerformanceCounter函数
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSencondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTime::TotalTime() const
{
	if (isStoped)
	{
		
		return (float)((mStopTime - mPauseTime - mBaseTime) * mSencondsPerCount);
	}
	else
	{
		return (float)((mCurrentTime - mPauseTime - mBaseTime) * mSencondsPerCount);
	}
}

float GameTime::DeltaTime() const
{
	return (float)mDeltaTime;
}

bool GameTime::IsStoped() const
{
	return isStoped;
}

void GameTime::Reset()
{
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

	mBaseTime = currentTime;     //当前时间为基准时间
	mPrevTime = mCurrentTime;	//因为重置 所以没有上一阵
	mStopTime = 0;
	isStoped = false;
}

void GameTime::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (isStoped) //如果是停止状态则让其解除停止
	{

		//计算暂停的总时间（次数）
		mPauseTime += (startTime - mStopTime);

		//修改停止状态
		mPrevTime = startTime; //相当于重置上一帧时刻
		mStopTime = 0;
		isStoped = false;
	}

	//如果不是停止状态，则什么也不用做
}

void GameTime::Stop()
{
	if (!isStoped)
	{
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

		mStopTime = currentTime;  //当前时间作为停止那一刻的时间(次数)
		isStoped = true;

	}
}

void GameTime::Tick()
{
	if (isStoped)
	{
		//如果当前是停止状态，则帧的间隔时间为0

		mDeltaTime = 0.0;
		return;

	}


	//计算当前的时刻值
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	mCurrentTime = currentTime;

	//计算当前帧和前一帧的时间差（计数器*每次多少秒）
	mDeltaTime = (mCurrentTime - mPrevTime) * mSencondsPerCount;

	//准备计算当前帧和下一帧的时间差
	mPrevTime = mCurrentTime;

	//排除时间差为负值
	if (mDeltaTime<0)
	{
		mDeltaTime = 0;
	}

}
