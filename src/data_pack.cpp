#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>

#include "utils.hpp"
#include "data_pack.hpp"

namespace zinx_asio {//namespace zinx_asio

std::size_t DataPack::maxPackageSize_ = 512;

DataPack::DataPack() {}
DataPack::~DataPack () {}

void DataPack::setMaxPackegeSize(std::size_t size) {
    maxPackageSize_ = size;
}

//GetHeadLen 获取头长度方法
uint32_t DataPack::getHeadLen()  {
    return 8;
}

//Pack 封包:len,ID,data,message放进dataBuf
void DataPack::pack(const void* source, void* dest, std::size_t size) {
    std::memcpy(dest, source, size);
}

void DataPack::pack(const void* source, std::vector<char>& dest, std::size_t size) {
    char*p = (char*)source;
    for (std::size_t i = 0; i < size; ++i) {
        dest.push_back(p[i]);
    }
}

void DataPack::pack(uint32_t len, uint32_t id, const void* source,
                    void* dest, std::size_t size) {
    std::size_t n = sizeof(len);
    std::memcpy((char*)dest, &len, n);
    std::memcpy((char*)dest + n, &id, n);
    std::memcpy((char*)dest + 2 * n, source, size);
}

//使用时注意dataBuf长度必须足够
void DataPack::pack(char* dataBuf, TCPMessage& msg) {
    int startPos = 0;
    //dataLen写进buf
    uint32_t len = msg.getMsgLen();
    std::memcpy(dataBuf + startPos, &len, sizeof(len));
    startPos += sizeof(uint32_t);
    //dataID写进buf
    uint32_t id = msg.getMsgID();
    std::memcpy(dataBuf + startPos, &id, sizeof(id));
    startPos += sizeof(uint32_t);
    //data写进buf
    msg.bufferRef().read(dataBuf + startPos, len);
}

//Pack 封包:len,ID,data
void DataPack::pack(boost::asio::streambuf& dataBuf, TCPMessage& msg) {
    std::ostream os(&dataBuf);
    //dataLen写进buf
    uint32_t len = msg.getMsgLen();
    os.write((char*)(&len), sizeof(len));
    //dataID写进buf
    uint32_t id = msg.getMsgID();
    os.write((char*)(&id), sizeof(id));
    //data写进buf
    os << msg.contentToString();
}

void DataPack::pack(std::string& dataBuf, TCPMessage& msg) {
    dataBuf.reserve(sizeof(uint32_t) + msg.getMsgLen());
    //dataLen写进buf
    uint32_t len = msg.getMsgLen();
    dataBuf.reserve(msg.getMsgLen() + getHeadLen());
    dataBuf[0] = (len >> 24) & 255;
    dataBuf[1] = (len >> 16) & 255;
    dataBuf[2] = (len >> 8) & 255;
    dataBuf[3] = len & 255;

    //dataID写进buf
    uint32_t id = msg.getMsgID();
    dataBuf[4] = (id >> 24) & 255;
    dataBuf[5] = (id >> 16) & 255;
    dataBuf[6] = (id >> 8) & 255;
    dataBuf[7] = id & 255;
    dataBuf += msg.contentToString();
}

void DataPack::pack(std::vector<char>& dataBuf, TCPMessage& msg) {
    dataBuf.reserve(sizeof(uint32_t) + msg.getMsgLen());
    //dataLen写进buf
    uint32_t len = msg.getMsgLen();
    dataBuf.reserve(msg.getMsgLen() + getHeadLen());
    dataBuf[0] = (len >> 24) & 255;
    dataBuf[1] = (len >> 16) & 255;
    dataBuf[2] = (len >> 8) & 255;
    dataBuf[3] = len & 255;

    //dataID写进buf
    uint32_t id = msg.getMsgID();
    dataBuf[4] = (id >> 24) & 255;
    dataBuf[5] = (id >> 16) & 255;
    dataBuf[6] = (id >> 8) & 255;
    dataBuf[7] = id & 255;
    for (auto i : msg.contentToString()) {
        dataBuf.push_back(i);
    }
}

//Unpack 拆包:读取数据包头
std::pair<uint32_t, uint32_t> DataPack::unpack(const char* dataBuf) {
    //读dataLen
    uint32_t len;
    std::memcpy((char*)(&len), dataBuf, 4);
    //读msgID
    uint32_t id;
    std::memcpy((char*)(&id), dataBuf + 4, 4);

    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
    return std::make_pair(len, id);
}

//Unpack 拆包:读取数据包头
std::pair<uint32_t, uint32_t> DataPack::unpack(boost::asio::streambuf& dataBuf) {
    std::iostream ios(&dataBuf);
    uint32_t len = 0;
    uint32_t id = 0;
    //读dataLen
    ios.read((char*)(&len), 4);
    //读msgID
    ios.read((char*)(&id), 4);

    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
    return std::make_pair(len, id);
}

//拆包:拿到msgLen和msgID
//Message的data中的前八个字节被取出,并拆包到message的len和id中
std::pair<uint32_t, uint32_t> DataPack::unpack(TCPMessage& msg) {
    uint32_t len;
    uint32_t id;
    msg.bufferRef().read(len).read(id);
    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
    msg.setMsgLen(len);
    msg.setMsgID(id);
    return std::make_pair(len, id);
}

//拆包:拿到msgLen和msgID
//char* 中数据不变
void DataPack::unpack(uint32_t &len, uint32_t &id, const char* dataBuf) {
    //读dataLen
    std::memcpy((char*)(&len), dataBuf, 4);
    //读msgID
    std::memcpy((char*)(&id), dataBuf + 4, 4);

    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
}

//拆包:拿到msgLen和msgID
//streambuf的前八个字节被取出
void DataPack::unpack(uint32_t &len, uint32_t &id, boost::asio::streambuf& dataBuf) {
    std::iostream ios(&dataBuf);
    //读dataLen
    ios.read((char*)(&len), 4);
    //读msgID
    ios.read((char*)(&id), 4);

    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
}

//拆包:拿到msgLen和msgID
//Message的data中的前八个字节被取出,并拆包到message的len和id中
void DataPack::unpack(uint32_t &len, uint32_t &id, TCPMessage& msg) {
    msg.bufferRef().read(len).read(id);
    if(GlobalObject::maxPackageSize() > 0
            && len > GlobalObject::maxPackageSize()) {
        throw std::logic_error("excess MaxPackageSize");
    }
    msg.setMsgLen(len);
    msg.setMsgID(id);
}

}//namespace zinx_asio
