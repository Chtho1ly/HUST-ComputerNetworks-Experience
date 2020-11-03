#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


SRSender::SRSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


SRSender::~SRSender()
{
}



bool SRSender::getWaitingState() {
	return waitingState;
}




bool SRSender::send(const Message& message) {
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}
	//���Ĺ���
	Packet temPack;
	temPack.acknum = -1; //���Ը��ֶ�
	temPack.seqnum = this->expectSequenceNumberSend;
	temPack.checksum = 0;
	memcpy(temPack.payload, message.data, sizeof(message.data));
	temPack.checksum = pUtils->calculateCheckSum(temPack);

	pUtils->printPacket("���ͷ����ͱ���", temPack);
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
		this->expectSequenceNumberSend = 0;
	pns->startTimer(SENDER, Configuration::TIME_OUT, temPack.seqnum);			//�������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, temPack);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

	this->packetWaitingAck.push_back(temPack);//����ǰ���͵����ݰ������ȷ�϶���β
	printf("���ͷ�: ");
	//�������
	if (!this->packetWaitingAck.empty())
	{
		int windowBegin = packetWaitingAck.front().seqnum;
		int windowEnd = (windowBegin + windowLen - 1) % seqLen;
		for (int i = 0; i < seqLen; i++)
		{
			if (windowBegin == i)
				printf("{");
			if (this->expectSequenceNumberSend == i)
				printf("|");
			printf(" %d ", i);
			if (windowEnd == i)
				printf("}");
		}
		printf("\n");
	}
	else
	{
		int windowBegin = this->expectSequenceNumberSend;
		int windowEnd = (windowBegin + windowLen - 1) % seqLen;
		for (int i = 0; i < seqLen; i++)
		{
			if (windowBegin == i)
				printf("{");
			if (this->expectSequenceNumberSend == i)
				printf("|");
			printf(" %d ", i);
			if (windowEnd == i)
				printf("}");
		}
		printf("\n");
	}
	//����һ��������Ŵ��ڻ���ڵ�һ����ȷ�����ݰ�����봰�ڳ���֮�ͣ������ȴ�״̬
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//����ȴ�״̬
	cout << endl;
	return true;
}

void SRSender::receive(const Packet& ackPkt) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//���У�����ȷ
	if (checkSum == ackPkt.checksum) {
		//���ACK���ǻظ��Ѿ�ȷ�ϵı���
		if (!this->packetWaitingAck.empty())
		{
			if (ackPkt.acknum >= this->packetWaitingAck.front().seqnum)
			{
				//�ٴ�ȷ�ϱ��Ķ�����Ѱ�Ҹ����
				for (auto i = this->packetWaitingAck.begin(); i < this->packetWaitingAck.end(); i++)
				{
					//���ҵ�����Ŷ�Ӧ���ģ������Ƴ���ֹͣ��Ӧ��ʱ��
					if (i->seqnum == ackPkt.acknum)
					{
						if (i == this->packetWaitingAck.begin())
							this->waitingState = false;
						pns->stopTimer(SENDER, i->seqnum);		//�رն�ʱ��
						this->packetWaitingAck.erase(i);//���ñ����Ƴ��ȴ�����
						break;
					}
				}

				pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);

				//�������
				printf("���ͷ�: ");
				if (!this->packetWaitingAck.empty())
				{
					int windowBegin = packetWaitingAck.front().seqnum;
					int windowEnd = (windowBegin + windowLen - 1) % seqLen;
					for (int i = 0; i < seqLen; i++)
					{
						if (windowBegin == i)
							printf("{");
						if (this->expectSequenceNumberSend == i)
							printf("|");
						printf(" %d ", i);
						if (windowEnd == i)
							printf("}");
					}
					printf("\n");
				}
				else
				{
					int windowBegin = this->expectSequenceNumberSend;
					int windowEnd = (windowBegin + windowLen-1) % seqLen;
					for (int i = 0; i < seqLen; i++)
					{
						if (windowBegin == i)
							printf("{");
						if (this->expectSequenceNumberSend == i)
							printf("|");
						printf(" %d ", i);
						if (windowEnd == i)
							printf("}");
					}
					printf("\n");
				}
				/*
				if (!this->packetWaitingAck.empty())
				{
					int windowBegin = packetWaitingAck.front().seqnum;
					int windowEnd = (this->packetWaitingAck.end() - 1)->seqnum;
					for (int i = 0; i < seqLen; i++)
					{
						if (windowBegin == i)
							printf("{");
						printf(" %d ", i);
						if (windowEnd == i)
							printf("}");
					}
					printf("\n");
				}
				else
				{
					for (int i = 0; i < seqLen; i++)
					{
						printf(" %d ", i);
						if (this->expectSequenceNumberSend == i)
							printf("{}");
					}
					printf("\n");
				}*/
			}
		}
	}
	cout << endl;
}

void SRSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	for (auto i : this->packetWaitingAck)
	{
		if (i.seqnum == seqNum)
		{
			pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط���ʱ�ı���", i);
			pns->sendToNetworkLayer(RECEIVER, i);			//���·������ݰ�
		}
	}
	cout << endl;
}
