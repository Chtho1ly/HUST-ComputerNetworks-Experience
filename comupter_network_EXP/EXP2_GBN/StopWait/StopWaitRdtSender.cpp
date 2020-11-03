#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


GBNSender::GBNSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


GBNSender::~GBNSender()
{
}



bool GBNSender::getWaitingState() {
	return waitingState;
}




bool GBNSender::send(const Message& message) {
	printf("���ͷ���sned\n");
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
	//���ȴ�����Ϊ�գ������з������ݰ�����ȷ��
	if (this->packetWaitingAck.empty())
		pns->startTimer(SENDER, Configuration::TIME_OUT, temPack.seqnum);			//�������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, temPack);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

	this->packetWaitingAck.push_back(temPack);//����ǰ���͵����ݰ������ȷ�϶���β
	//seq++
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
		this->expectSequenceNumberSend = 0;
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
		for (int i = 0; i < seqLen; i++)
		{
			printf(" %d ", i);
			if (this->expectSequenceNumberSend == i)
				printf("{}");
		}
		printf("\n");
	}
	//����һ��������Ŵ��ڻ���ڵ�һ����ȷ�����ݰ�����봰�ڳ���֮�ͣ������ȴ�״̬
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//����ȴ�״̬
	cout << endl;
	return true;
}

void GBNSender::receive(const Packet& ackPkt) {
	printf("���ͷ���receive\n");
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//���У�����ȷ
	if (checkSum == ackPkt.checksum) {
		if (!this->packetWaitingAck.empty())
		{
			if (!(ackPkt.acknum == seqLen - 1 && this->packetWaitingAck.front().seqnum == 0))
			{
				//�������δȷ�ϱ���
				if (!this->packetWaitingAck.empty())
				{
					//��ȷ�ϵı���seqnum��û����
					if (ackPkt.acknum >= this->packetWaitingAck.front().seqnum)
					{
						//ֹͣ��ʱ
						pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);		//�رն�ʱ��
						this->waitingState = false;
						//ȷ��ACK֮ǰ�ı���
						this->packetWaitingAck.erase(this->packetWaitingAck.begin(), this->packetWaitingAck.begin() + (ackPkt.acknum - this->packetWaitingAck.front().seqnum + 1));
						//�Ե�һ���ȴ�ȷ�ϱ������Ϊ����������ʱ
						if (!this->packetWaitingAck.empty())
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);
						pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
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
					}
					//��ȷ�ϱ��ĵ�seqnum�Ѿ����꣬���´�0��ʼ
					else if (ackPkt.acknum < this->packetWaitingAck.front().seqnum + windowLen - seqLen)
					{
						//ֹͣ��ʱ
						pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);		//�رն�ʱ��
						this->waitingState = false;
						//ȷ��ACK֮ǰ�ı���
						this->packetWaitingAck.erase(this->packetWaitingAck.begin(), this->packetWaitingAck.begin() + (ackPkt.acknum + seqLen - this->packetWaitingAck.front().seqnum + 1));
						//�Ե�һ���ȴ�ȷ�ϱ������Ϊ����������ʱ
						if (!this->packetWaitingAck.empty())
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);
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
					}
				}
			}
		}
	}
	cout << endl;
}

void GBNSender::timeoutHandler(int seqNum) {
	printf("���ͷ���timeout\n");
	//Ψһһ����ʱ��,���迼��seqNum
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	for (auto i : this->packetWaitingAck)
	{
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط����еȴ�ȷ�ϵı���", i);
		pns->sendToNetworkLayer(RECEIVER, i);			//���·������ݰ�
	}
	cout << endl;
}
