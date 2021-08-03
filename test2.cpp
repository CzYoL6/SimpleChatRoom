#include <iostream>
#include "message.pb.h"
#include "packet.h"

int main() {
    Packet* packet = new Packet(Chat::TYPE::chatMessage_C_TO_S);
    std::cout << "size: " << sizeof(Chat::TYPE) << std::endl;
    std::string msg{"hello there!"};
    Chat::ChatMessage_C_TO_S protoMsg;
    protoMsg.set_msg(msg);
    std::cout << "protobytesize: " << protoMsg.ByteSizeLong() << std::endl;
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    protoMsg.SerializeToArray(tmp, protoMsg.ByteSizeLong());
    packet->AddVal(tmp, protoMsg.ByteSizeLong());
    packet->AddLength();

    std::cout << "packet length: " << packet->GetAllLength() << std::endl;

    if (packet->GetBuffer()->GetLength() >= sizeof(unsigned int)) {
        char tmp[1024];
        memset(tmp, 0, sizeof(tmp));
        int read_res =
            packet->GetBuffer()->Read(tmp, sizeof(unsigned int), true);
        unsigned int p_tmp_res = *(reinterpret_cast<unsigned int*>(tmp));
        std::cout << "read_res: " << read_res << " "
                  << " tmp_res: " << p_tmp_res << std::endl;

        if (packet->GetBuffer()->GetLength() >=
            sizeof(unsigned int) + p_tmp_res) {
            memset(tmp, 0, sizeof(tmp));
            packet->GetBuffer()->Read(tmp, sizeof(unsigned int));
            memset(tmp, 0, sizeof(tmp));
            packet->GetBuffer()->Read(tmp, sizeof(Chat::TYPE));
            std::cout << "type: " << *reinterpret_cast<Chat::TYPE*>(tmp)
                      << std::endl;

            memset(tmp, 0, sizeof(tmp));
            packet->GetBuffer()->Read(tmp, p_tmp_res - sizeof(Chat::TYPE));
            std::cout << p_tmp_res - sizeof(Chat::TYPE) << std::endl;
            Chat::ChatMessage_C_TO_S newMsg;
            if (newMsg.ParseFromArray(tmp, p_tmp_res - sizeof(Chat::TYPE)))
                std::cout << "msg: " << newMsg.msg() << std::endl;
            else {
                std::cout << "parse failed." << std::endl;
            }
        }
    }
}