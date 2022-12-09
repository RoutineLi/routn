/*************************************************************************
	> File Name: worker.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年11月28日 星期一 00时10分34秒
 ************************************************************************/

#include "Worker.h"
#include "Config.h"

namespace Routn{
		
	static Routn::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_worker_config
		= Routn::Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string> >(), "worker config");

	WorkerGroup::WorkerGroup(uint32_t batch_size, Schedular* s)
		:m_batchSize(batch_size)
		,m_finish(false)
		,m_scheduler(s)
		,m_sem(batch_size) {
	}

	WorkerGroup::~WorkerGroup() {
		waitAll();
	}
	void WorkerGroup::schedule(const std::vector<std::function<void()> >& cbs) {
		std::vector<std::function<void()> > cs;
		for(auto& i : cbs) {
			cs.push_back(std::bind(&WorkerGroup::doWork
							,shared_from_this(), i));
			m_sem.wait();
		}
		m_scheduler->schedule(cs.begin(), cs.end());
	}

	void WorkerGroup::schedule(std::function<void()> cb, int thread) {
		m_sem.wait();
		m_scheduler->schedule(std::bind(&WorkerGroup::doWork
							,shared_from_this(), cb), thread);
	}

	void WorkerGroup::doWork(std::function<void()> cb) {
		cb();
		m_sem.notify();
	}

	void WorkerGroup::waitAll() {
		if(!m_finish) {
			m_finish = true;
			for(uint32_t i = 0; i < m_batchSize; ++i) {
				m_sem.wait();
			}
		}
	}

	WorkerManager::WorkerManager()
		:m_stop(false) {
	}

	void WorkerManager::add(Schedular::ptr s) {
		m_datas[s->getName()].push_back(s);
	}

	void WorkerManager::add(const std::string& name, Schedular::ptr s){
		m_datas[name].push_back(s);	
	}

	Schedular::ptr WorkerManager::get(const std::string& name) {
		srand(time(0));
		auto it = m_datas.find(name);
		if(it == m_datas.end()) {
			return nullptr;
		}
		if(it->second.size() == 1) {
			return it->second[0];
		}
		return it->second[rand() % it->second.size()];
	}

	IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
		return std::dynamic_pointer_cast<IOManager>(get(name));
	}

	bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string> >& v) {
		for(auto& i : v) {
			std::string name = i.first;
			int32_t thread_num = Routn::GetParamValue(i.second, "thread_num", 1);
			int32_t worker_num = Routn::GetParamValue(i.second, "worker_num", 1);
			

			for(int32_t x = 0; x < worker_num; ++x) {
				Schedular::ptr s;
				if(!x) {
					s = std::make_shared<IOManager>(thread_num, false, name);
				} else {
					s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(x));
				}
				add(name, s);
			}
		}
		m_stop = m_datas.empty();
		return true;
	}

	bool WorkerManager::init() {
		auto workers = g_worker_config->getValue();
		return init(workers);
	}

	void WorkerManager::stop() {
		if(m_stop) {
			return;
		}
		for(auto& i : m_datas) {
			for(auto& n : i.second) {
				n->schedule([](){});
				n->stop();
			}
		}
		m_datas.clear();
		m_stop = true;
	}

	uint32_t WorkerManager::getCount() {
		return m_datas.size();
	}

}

