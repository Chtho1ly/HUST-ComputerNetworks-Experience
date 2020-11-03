#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"
#define windowLen 4
#define seqLen 8


SRReceiver::SRReceiver() :expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRReceiver::~SRReceiver()
{
}

void SRReceiver::receive(const Packet& packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ
	if (checkSum == packet.checksum)
	{
		//�����Ļ�δ�յ���
		if ((this->expectSequenceNumberRcvd <= packet.seqnum && this->expectSequenceNumberRcvd + windowLen > packet.seqnum) || packet.seqnum < this->expectSequenceNumberRcvd + windowLen - seqLen)
		{
			//�����ܱ���Ϊ�������ģ����䴫�ݸ�Ӧ�ò㣬����������������֮������ݴ��ݸ�Ӧ�ò�
			if (this->expectSequenceNumberRcvd == packet.seqnum)
			{
				pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
				//ȡ��Message�����ϵݽ���Ӧ�ò�
				Message msg;
				memcpy(msg.data, packet.payload, sizeof(packet.payload));
				pns->delivertoAppLayer(RECEIVER, msg);
				this->expectSequenceNumberRcvd++;
				if (this->expectSequenceNumberRcvd == seqLen)
					this->expectSequenceNumberRcvd = 0;
				//ɨ�軺����
				while (true)
				{
					auto i = this->buffer.begin();
					for (; i < this->buffer.end(); i++)
					{
						if (i->seqnum == this->expectSequenceNumberRcvd)
							break;
					}
					if (i == this->buffer.end())
						break;
					memcpy(msg.data, i->payload, sizeof(i->payload));
					pns->delivertoAppLayer(RECEIVER, msg);
					this->buffer.erase(i);
					this->expectSequenceNumberRcvd++;
					if (this->expectSequenceNumberRcvd == seqLen)
						this->expectSequenceNumberRcvd = 0;
				}
				//�������
				printf("���շ�: ");
				int windowBegin = this->expectSequenceNumberRcvd;
				int windowEnd = (windowBegin + windowLen - 1) % seqLen;
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

			//�����ձ�������������֮���һ�������û����ͬ���ݣ�������뻺����
			else
			{
				this->buffer.push_back(packet);
				sort(this->buffer.begin(), this->buffer.end(), [](Packet a, Packet b) {return a.seqnum < b.seqnum; });
				auto i = unique(this->buffer.begin(), this->buffer.end(), [](Packet a, Packet b) {return a.seqnum == b.seqnum; });
				this->buffer.erase(i, this->buffer.end());
				pUtils->printPacket("���շ���ȷ�յ����ͷ�ʧ��ı���", packet);
				//�������
				printf("���շ�: ");
				int windowBegin = this->expectSequenceNumberRcvd;
				int windowEnd = (windowBegin + windowLen - 1) % seqLen;
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
		}
		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
	}
	cout << endl;
}