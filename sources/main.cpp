#include <chrono>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <cmath>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <numeric>
#include <filesystem>

using namespace std;

int main()
{
	string inPath = "../../in";
	string outPath = "../../out";
	uint64_t scoreTotal = 0;
	uint64_t millisecondsTotal = 0;

	for (const auto& taskPath : experimental::filesystem::v1::directory_iterator(inPath))
	{
		chrono::time_point<std::chrono::system_clock> timeStart =
			chrono::system_clock::now();

		stringstream ss;
		ss << taskPath;
		string filePathIn = ss.str();

		size_t fileNameStart = filePathIn.find_last_of('\\');
		size_t fileNameEnd = filePathIn.find_last_of('.');
		string fileName = filePathIn.substr(fileNameStart + 1, fileNameEnd - fileNameStart - 1);

		string filePathOut = outPath + "/" + fileName + ".out";

		// input
		ifstream fileIn;
		fileIn.open(filePathIn);
		assert(fileIn.is_open());

		using uint = uint64_t;
		uint videosCount = 0;
		uint endpointsCount = 0;
		uint requestsCount = 0;
		uint cachesCount = 0;
		uint cachePayload = 0;
		vector<uint> videosSizes;

		struct EndpointCache
		{
			uint cacheId = 0;
			uint latency = 0;
		};

		struct Endpoint
		{
			uint latencyDatacenter = 0;
			uint cachesConnected = 0;
			vector<EndpointCache> caches;

			set<uint> requestsId; // by id
		};

		vector<Endpoint> endpoints;

		struct Request
		{
			uint id = 0;

			uint count = 0;
			uint videoId = 0;
			uint endpointId = 0;
		};

		vector<Request> requests;

		fileIn >> videosCount >> endpointsCount >> requestsCount >> cachesCount >> cachePayload;

		for (uint i = 0; i < videosCount; ++i)
		{
			uint size = 0;
			fileIn >> size;
			videosSizes.emplace_back(size);
		}

		for (uint i = 0; i < endpointsCount; ++i)
		{
			Endpoint e;
			fileIn >> e.latencyDatacenter >> e.cachesConnected;

			for (uint c = 0; c < e.cachesConnected; ++c)
			{
				EndpointCache ec;
				fileIn >> ec.cacheId >> ec.latency;
				e.caches.emplace_back(ec);
			}

			endpoints.emplace_back(e);
		}

		for (uint i = 0; i < requestsCount; ++i)
		{
			Request r;
			r.id = i;
			fileIn >> r.videoId >> r.endpointId >> r.count;

			endpoints[r.endpointId].requestsId.insert(r.id);

			requests.emplace_back(r);
		}

		fileIn.close();

		// solve
		map<uint, set<uint>> result; // key = cacheid , value = videoIds

		struct Cache
		{
			uint cacheId = 0;

			set<uint> videoIds; // result
			uint payload = 0;

			set<uint> endpointsConnected; // by id
			set<uint> videoIdsPending; // by id
			
			map<uint, uint> videoScores; // key = id
			
			set<uint> videoIdsLoaded; // key = id
			uint scoreTotal = 0;
		};

		map<uint, Cache> caches; // key = id cache

		for (uint endpointId = 0; endpointId < endpointsCount; ++endpointId)
		{
			for (const auto& cache : endpoints[endpointId].caches)
			{
				caches[cache.cacheId].cacheId = cache.cacheId;
				caches[cache.cacheId].endpointsConnected.insert(endpointId);
			}
		}

		for (auto&& cache : caches)
		{
			for (auto&& endpointId : cache.second.endpointsConnected)
			{
				for (auto&& requestId : endpoints[endpointId].requestsId)
				{
					cache.second.videoIdsPending.insert(requests[requestId].videoId);
				}
			}
		}

		for (auto&& cache : caches)
		{
			for (auto&& videoId : cache.second.videoIdsPending)
			{
				uint videoScore = 0;

				for (auto&& endpointId : cache.second.endpointsConnected)
				{
					for (auto&& requestId : endpoints[endpointId].requestsId)
					{
						if (requests[requestId].videoId == videoId)
						{
							videoScore += (endpoints[endpointId].latencyDatacenter - 
								endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[requestId].count;
						}
					}
				}

				cache.second.videoScores[videoId] = videoScore;
			}
		}





		// output
		ofstream fileOut;
		fileOut.open(filePathOut);
		assert(fileOut.is_open());

		fileOut << result.size() << endl;

		for (const auto& cacheIt : result)
		{
			fileOut << cacheIt.first;

			for (const auto& videoIt : cacheIt.second)
			{
				fileOut << " " << videoIt;
			}
			
			fileOut << endl;
		}

		fileOut.close();

		// score
		uint64_t score = 0;
		scoreTotal += score;
		// timing
		chrono::time_point<std::chrono::system_clock> timeEnd =
			chrono::system_clock::now();
		chrono::milliseconds millisec = chrono::duration_cast<chrono::milliseconds>(timeEnd - timeStart);
		millisecondsTotal += millisec.count();
		cout << "file: " << fileName << endl;
		cout << "score: " << score << endl;
		cout << "time(ms): " << to_string(millisec.count()) << endl << endl;
	}

	cout << "scoreTotal: " << scoreTotal << endl;
	cout << "millisecondsTotal: " << millisecondsTotal << endl << endl;
	system("pause");

	return 0;
}