#include <assert.h>
#include "message.pb.h"
#include "packet.h"
#include "server.h"

namespace SERVER_SEND {
void Welcome(ServerClient* cli) {
    std::cout << "sending welcome msg..." << std::endl;
    Packet* packet = new Packet(Chat::TYPE::welcome_S_TO_C);

    Chat::Welcome_S_TO_C msg;
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));
    msg.SerializeToArray(tmp, msg.ByteSizeLong());

    packet->AddVal(tmp, msg.ByteSizeLong());
    packet->InsertLengthInFront();
    cli->WriteIntoSendBuffer(packet->GetCircleBuffer()->GetBuffer(),
                             packet->GetAllLength());
    delete packet;
}

void NicknameSet(ServerClient* cli) {
    Packet* packet = new Packet(Chat::TYPE::nickNameBeenSet_S_TO_C);
    Chat::NickName_Been_Set_S_TO_C protoMsg;
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));
    protoMsg.SerializeToArray(tmp, protoMsg.ByteSizeLong());
    packet->AddVal(tmp, protoMsg.ByteSizeLong());
    packet->InsertLengthInFront();

    cli->WriteIntoSendBuffer(packet->GetCircleBuffer()->GetBuffer(),
                             packet->GetAllLength());

    delete packet;
}
}  // namespace SERVER_SEND