//
// Created by Dmitry Isaev on 2019-03-12.
//

#include "GetRangeHandler.h"
#include "../IDComparator.h"
#include "spdlog/spdlog.h"
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include "../utils/RepositoryUtils.h"
#include "Poco/BinaryWriter.h"
#include "Poco/Net/MessageHeader.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::MessageHeader;


namespace timemachine {
    namespace handlers {

        GetRangeHandler::GetRangeHandler(
                std::shared_ptr<timemachine::DbClient>_client,
                std::string&& _name,
                unsigned long int _from,
                unsigned long int _till,
                unsigned long int _max_messages,
                unsigned long int _max_bytes,
                bool _reverse) :
        client(_client), name(_name), from(_from), till(_till), maxBytes(_max_bytes), maxMessages(_max_messages), reverse(_reverse), timemachine::utils::RepositoryUtils(){
            readOptions = rocksdb::ReadOptions();
        }

        void GetRangeHandler::handleRequest(HTTPServerRequest &request, HTTPServerResponse &response){
            response.setChunkedTransferEncoding(true);
            response.setContentType("application/octet-stream");
            std::ostream &ostr = response.send();

            response.setChunkedTransferEncoding(true);

            if (request.has("Access-Control-Request-Method")){
				const std::string& method = request.get("Access-Control-Request-Method");

				response.set("Access-Control-Allow-Method", method);

			}
			if (request.has("Access-Control-Request-Headers")){
				const std::string& headers = request.get("Access-Control-Request-Headers");
				std::vector<std::string> list;
				MessageHeader::splitElements(headers, list);
				std::string allowHeaders;
				for (auto it = list.begin(); it != list.end(); ++it){

					if(!allowHeaders.empty()){
						allowHeaders += ", ";
					}
					allowHeaders += *it;
				}
				response.set("Access-Control-Allow-Headers", allowHeaders);
			}

            spdlog::debug("Get range from {0} to {1}", from, till);
            auto cf = client->GetOrCreateColumnFamily(name);

            spdlog::debug("getting iterator from {0} to {1}", from, till);

            ID keyFrom;
            keyFrom.set_timestamp(from);

            char bytes[16];
            SerializeID(&keyFrom, bytes);
            auto keyFromSlice = rocksdb::Slice(bytes, 16);

            spdlog::debug("iterating with: reverse = {}, from = {}, till = {}, maxMessages = {}, maxBytes = {}", reverse, from, till, maxMessages, maxBytes);

            timemachine::RangeRequest req;
            req.set_from(from);
            req.set_till(till);
            req.set_reverse(reverse);
            req.set_maxmessages(maxMessages);
            req.set_maxbytes(maxBytes);

            const timemachine::RangeRequest* reqRef = &req;

            client->Iter(readOptions, cf, reqRef, [&](timemachine::ID key, timemachine::Data data, bool stopIt)->unsigned long int {

                auto ulongSize = sizeof(long int);
                auto uintSize = sizeof(int);
                auto totalSize = ulongSize * 2 + uintSize;
                long int ts_ = (long)key.timestamp();
                long int unique_ = (long)key.unique();
                auto body = data.data();
                int size_ = body.size();

                spdlog::debug("batch to send: \n ts: {} \n unique: {} \n size: {}", ts_, unique_, size_);
                auto br = Poco::BinaryWriter(ostr, Poco::BinaryWriter::NETWORK_BYTE_ORDER);
                br << ts_ << unique_ << size_;
                ostr << body;

                return size_ + ulongSize + ulongSize + uintSize;

            });

            response.setStatus(Poco::Net::HTTPServerResponse::HTTP_OK);

        }

    }
}
