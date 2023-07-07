#include "videosurface.h"
#include "media.h"
#include <shlwapi.h>

#include <wincodec.h>

namespace media
{
	uint32_t SurfaceManager::Add(const VIDEO_SURFACE& item)
	{
		uint32_t id;
		if (m_freelist.empty())
		{
			id = m_nextId++;
		}
		else
		{
			id = m_freelist.front();
			m_freelist.pop_front();
		}
		m_items.insert(std::make_pair(id, item));
		return id;
	}

	void SurfaceManager::Remove(const uint32_t id)
	{
		auto it = m_items.find(id);
		if (it != m_items.end() && id < m_nextId)
		{
			if (id == m_nextId - 1)
			{
				m_nextId--;
				return;
			}
			m_items.erase(it);
			m_freelist.push_back(id);
		}
		else throw std::out_of_range::exception();
	}

	VIDEO_SURFACE& SurfaceManager::Get(const uint32_t id)
	{
		auto it = m_items.find(id);
		if (it != m_items.end())
			return it->second;
		else
			throw std::out_of_range("Invalid ID");
	}

	void SurfaceManager::Clear()
	{
		m_nextId = 1;
		m_lastDeallocatedId = 0xffffffff;
		m_freelist.clear();
		m_items.clear();
	}

	bool SurfaceManager::IsValid(const uint32_t id) const
	{
		return id != 0xffffffff &&
			id < m_nextId && m_items.find(id) != m_items.end();
	}
}