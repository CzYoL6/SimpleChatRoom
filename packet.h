#include "circle_buffer.hpp"
#include "message.pb.h"
class Packet {
   public:
    Packet(Chat::TYPE type);
    ~Packet();

    void AddVal(char* val, int size);

    void ReadVal(char* val, int size, bool peek);

    int AddLength();

    int GetAllLength() const;

    CircleBuffer<char>* GetBuffer() const;

   private:
    CircleBuffer<char>* m_buffer{nullptr};
    Chat::TYPE m_type;
};