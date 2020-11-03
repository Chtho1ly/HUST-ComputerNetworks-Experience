#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"
#define windowLen 4
#define seqLen 8


TCPSender::TCPSender() :expectSequenceNumberSend(0), waitingState(false)
{
}


TCPSender::~TCPSender()
{
}



bool TCPSender::getWaitingState() {
	return waitingState;
}




bool TCPSender::send(const Message& message) {
	printf("���ͷ���send\n");
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
	this->expectSequenceNumberSend++;
	if (this->expectSequenceNumberSend == seqLen)
	{
		this->qResendCnt = 0;
		this->expectSequenceNumberSend = 0;
	}
	//����һ��������Ŵ��ڻ���ڵ�һ����ȷ�����ݰ�����봰�ڳ���֮�ͣ������ȴ�״̬
	if (this->expectSequenceNumberSend >= this->packetWaitingAck.front().seqnum + windowLen || this->expectSequenceNumberSend <= this->packetWaitingAck.front().seqnum + windowLen - seqLen)
		this->waitingState = true;																					//����ȴ�״̬
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
	cout << endl;
	return true;
}

void TCPSender::receive(const Packet& ackPkt) {
	printf("���ͷ���receive\n");
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//���У�����ȷ
	if (checkSum == ackPkt.checksum) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		//���ACK���ǻظ��Ѿ�ȷ�ϵı��ģ��򴰿�һ������
		if (!this->packetWaitingAck.empty())
		{
			if (!(ackPkt.acknum == seqLen - 1 && this->packetWaitingAck.front().seqnum == 0))
			{
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
				//ACK�ظ���ȷ�ϱ���
				else if (ackPkt.acknum == this->packetWaitingAck.front().seqnum - 1)
				{
					if (this->qResendSeq == ackPkt.acknum)
					{
						this->qResendCnt++;
						//������������ͬ����ACK����Ҫ�����ش�
						if (this->qResendCnt == 3)
						{
							this->qResendCnt = 0;
							pns->stopTimer(SENDER, this->packetWaitingAck.front().seqnum);										//���ȹرն�ʱ��
							pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.front().seqnum);			//�����������ͷ���ʱ��
							pUtils->printPacket("���ͷ��յ�3����ͬ����ACK��ʼ�����ش�", this->packetWaitingAck.front());
							pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.front());			//���·������ݰ�
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
					else
					{
						this->qResendSeq = ackPkt.acknum;
						this->qResendCnt = 1;
					}
				}
			}
		}
	}
	cout << endl;
}

void TCPSender::timeoutHandler(int seqNum) {
	printf("���ͷ���timeout\n");
	//Ψһһ����ʱ��,���迼��seqNum
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ȴ�ȷ�ϵĵ�һ������", this->packetWaitingAck.front());
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.front());			//���·������ݰ�
	cout << endl;
}
