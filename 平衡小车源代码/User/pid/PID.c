#include "stm32f10x.h"                  // Device header
#include "PID.h"

void PID_Update(PID_t *p)
{
	float error;
	float errorIntNew;
	float out;

	p->Error1 = p->Error0;
	p->Error0 = p->Target - p->Actual;

	error = p->Error0;
	errorIntNew = p->ErrorInt + error;

	out = p->Kp * error
		+ p->Ki * errorIntNew
		+ p->Kd * (error - p->Error1);

	/* 只有未饱和，或误差正在帮助输出回到范围内时才积分 */
	if ((out < p->OutMax && out > p->OutMin) ||
		(out >= p->OutMax && error < 0) ||
		(out <= p->OutMin && error > 0))
	{
		p->ErrorInt = errorIntNew;
	}

	p->Out = p->Kp * error
		+ p->Ki * p->ErrorInt
		+ p->Kd * (error - p->Error1);

	if (p->Out > p->OutMax) p->Out = p->OutMax;
	if (p->Out < p->OutMin) p->Out = p->OutMin;
}
void PID_Init(PID_t *p)
{
	p->Target = 0;
	p->Actual = 0;
	p->Out = 0;
	p->Error0 = 0;
	p->Error1 = 0;
	p->ErrorInt = 0;
}

