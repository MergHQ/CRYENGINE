// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once
#include "VKBase.hpp"

namespace NCryVulkan
{
	class CShader : public CDeviceObject
	{
	public:
		CShader(CDevice* pDevice);
		virtual ~CShader();

		VkResult Init(const void* pShaderCode, size_t shaderCodeSize);
		VkShaderModule GetVulkanShader() const { return m_shaderHandle; }
		void SetDebugName(const char* szName) { /*TODO*/ }
		const char* GetDebugName() { /*TODO*/ return ""; }

	private:
		VkShaderModule m_shaderHandle;
	};

}
