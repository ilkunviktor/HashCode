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
			map<uint, EndpointCache> caches;

			map<uint, uint> videoToRequests;
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
				//e.caches.emplace_back(ec);
				e.caches[ec.cacheId] = ec;
			}

			endpoints.emplace_back(e);
		}

		for (uint i = 0; i < requestsCount; ++i)
		{
			Request r;
			r.id = i;
			fileIn >> r.videoId >> r.endpointId >> r.count;

			endpoints[r.endpointId].requestsId.insert(r.id);
			
			auto videoRequestIt = endpoints[r.endpointId].videoToRequests.find(r.videoId);			
			endpoints[r.endpointId].videoToRequests[r.videoId] = r.id;						

			requests.emplace_back(r);
		}

		fileIn.close();

		// solve
		map<uint, set<uint>> result; // key = cacheid , value = videoIds

		struct Cache
		{
			uint cacheId = 0;

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
				caches[cache.first].cacheId = cache.first;
				caches[cache.first].endpointsConnected.insert(endpointId);
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
					const auto requestId = endpoints[endpointId].videoToRequests.find(videoId);

					if (requestId != endpoints[endpointId].videoToRequests.end())
					{
						videoScore += (endpoints[endpointId].latencyDatacenter -
							endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[requestId->second].count;
					}


					/*auto requestIdIt = find_if(endpoints[endpointId].requestsId.begin(),
					endpoints[endpointId].requestsId.end(),
					[&requests, &videoId](const uint& requestId)
					{
					return requests[requestId].videoId == videoId;
					});

					if (requestIdIt != endpoints[endpointId].requestsId.end())
					{
					videoScore += (endpoints[endpointId].latencyDatacenter -
					endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[*requestIdIt].count;
					}*/
					/*
					for (auto&& requestId : )
					{
					if (requests[requestId].videoId == videoId)
					{
					videoScore += (endpoints[endpointId].latencyDatacenter -
					endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[requestId].count;
					}
					}
					*/
				}

				cache.second.videoScores[videoId] = videoScore;
			}
		}

		while (!caches.empty())
		{
			/*for (auto&& cache : caches)
			{
				for (auto&& endpointId : cache.second.endpointsConnected)
				{
					for (auto&& requestId : endpoints[endpointId].requestsId)
					{
						cache.second.videoIdsPending.insert(requests[requestId].videoId);
					}
				}
			}*/

			//for (auto&& cache : caches)
			//{
			//	for (auto&& videoId : cache.second.videoIdsPending)
			//	{
			//		uint videoScore = 0;

			//		for (auto&& endpointId : cache.second.endpointsConnected)
			//		{
			//			const auto requestId = endpoints[endpointId].videoToRequests.find(videoId);

			//			if (requestId != endpoints[endpointId].videoToRequests.end())
			//			{							
			//				videoScore += (endpoints[endpointId].latencyDatacenter -
			//					endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[requestId->second].count;
			//			}


			//			/*auto requestIdIt = find_if(endpoints[endpointId].requestsId.begin(),
			//				endpoints[endpointId].requestsId.end(),
			//				[&requests, &videoId](const uint& requestId)
			//			{
			//				return requests[requestId].videoId == videoId;
			//			});

			//			if (requestIdIt != endpoints[endpointId].requestsId.end())
			//			{
			//				videoScore += (endpoints[endpointId].latencyDatacenter -
			//					endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[*requestIdIt].count;
			//			}*/
			//			/*
			//			for (auto&& requestId : )
			//			{
			//				if (requests[requestId].videoId == videoId)
			//				{
			//					videoScore += (endpoints[endpointId].latencyDatacenter -
			//						endpoints[endpointId].caches[cache.second.cacheId].latency) * requests[requestId].count;
			//				}
			//			}
			//			*/
			//		}

			//		cache.second.videoScores[videoId] = videoScore;
			//	}
			//}

			auto FindCacheScore = [](Cache& c)
			{
				uint max = 0;
				uint indexMax = 0;
				for (auto&& score : c.videoScores)
				{
					if (score.second>max)
					{
						indexMax = score.first;
						max = score.second;
					}
				}

				c.videoIdsLoaded.insert(indexMax);
				c.scoreTotal = c.videoScores[indexMax];
			};

	/*		auto FindCacheScore = [](Cache& c)
			{
				uint videoId = *c.videoIdsPending.begin();
				c.videoIdsLoaded.insert(videoId);

				c.scoreTotal = c.videoScores[videoId];
			};*/

			for (auto&& cache : caches)
			{
				FindCacheScore(cache.second);
			}

			uint cacheIdScoreMax = -1;
			uint cachScoreTotalMax = 0;

			for (auto&& cache : caches)
			{
				if (cache.second.scoreTotal > cachScoreTotalMax)
				{
					cachScoreTotalMax = cache.second.scoreTotal;
					cacheIdScoreMax = cache.second.cacheId;
				}
			}

			auto cacheIt = caches.find(cacheIdScoreMax);

			if (cacheIt != caches.end())
			{
				result[cacheIt->second.cacheId] = cacheIt->second.videoIdsLoaded;
				caches.erase(cacheIt);
			}
			else
			{
				break;
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
		uint score = 0;

		uint requestsTotalCount = 0;

		for (const auto& r : requests)
		{
			requestsTotalCount += r.count;
		}

		uint reqUp = 0;

		for (const auto& r : requests)
		{
			//r.videoId
			uint requestLatency = endpoints[r.endpointId].latencyDatacenter;

			for (const auto& cache : endpoints[r.endpointId].caches)
			{
				auto cacheResultedIt = result.find(cache.first);

				if (cacheResultedIt != result.end())
				{
					auto videoIt = cacheResultedIt->second.find(r.videoId);

					if (videoIt != cacheResultedIt->second.end())
					{
						uint cacheLatency = endpoints[r.endpointId].caches[cacheResultedIt->first].latency;

						if (cacheLatency < requestLatency)
						{
							requestLatency = cacheLatency;
						}
					}
				}
			}

			reqUp += (endpoints[r.endpointId].latencyDatacenter - requestLatency) * r.count;
		}

		score = (uint)floor((double)reqUp / (double)requestsTotalCount);
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