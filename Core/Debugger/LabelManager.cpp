#include "stdafx.h"
#include "LabelManager.h"
#include "Debugger.h"
#include "DebugUtilities.h"
#include "DebugBreakHelper.h"

LabelManager::LabelManager(Debugger *debugger)
{
	_debugger = debugger;
}

void LabelManager::ClearLabels()
{
	DebugBreakHelper helper(_debugger);
	_codeLabels.clear();
	_codeLabelReverseLookup.clear();
}

void LabelManager::SetLabel(uint32_t address, SnesMemoryType memType, string label, string comment)
{
	DebugBreakHelper helper(_debugger);
	uint64_t key = GetLabelKey(address, memType);

	auto existingLabel = _codeLabels.find(key);
	if(existingLabel != _codeLabels.end()) {
		_codeLabelReverseLookup.erase(existingLabel->second.Label);
	}

	_codeLabels.erase(key);
	if(!label.empty() || !comment.empty()) {
		if(label.size() > 400) {
			//Restrict labels to 400 bytes
			label = label.substr(0, 400);
		}

		LabelInfo labelInfo;
		labelInfo.Label = label;
		labelInfo.Comment = comment;

		_codeLabels.emplace(key, labelInfo);
		_codeLabelReverseLookup.emplace(label, key);
	}
}

int64_t LabelManager::GetLabelKey(uint32_t absoluteAddr, SnesMemoryType memType)
{
	return absoluteAddr | ((uint64_t)memType << 32);
}

SnesMemoryType LabelManager::GetKeyMemoryType(uint64_t key)
{
	return (SnesMemoryType)(key >> 32);
}

string LabelManager::GetLabel(AddressInfo address)
{
	string label;
	if(address.Type <= DebugUtilities::GetLastCpuMemoryType()) {
		if(address.Type == SnesMemoryType::GameboyMemory) {
			//Labels for GB registers
			if(InternalGetLabel(address, label)) {
				return label;
			}
		}
		address = _debugger->GetAbsoluteAddress(address);
	}

	if(address.Address >= 0) {
		InternalGetLabel(address, label);
	}

	return label;
}

bool LabelManager::InternalGetLabel(AddressInfo address, string &label)
{
	int64_t key = GetLabelKey(address.Address, address.Type);
	if(key >= 0) {
		auto result = _codeLabels.find(key);
		if(result != _codeLabels.end()) {
			label = result->second.Label;
			return true;
		}
	}
	return false;
}

string LabelManager::GetComment(AddressInfo absAddress)
{
	uint64_t key = GetLabelKey(absAddress.Address, absAddress.Type);

	if(key >= 0) {
		auto result = _codeLabels.find(key);
		if(result != _codeLabels.end()) {
			return result->second.Comment;
		}
	}

	return "";
}

bool LabelManager::GetLabelAndComment(AddressInfo address, LabelInfo &labelInfo)
{
	if(address.Type <= DebugUtilities::GetLastCpuMemoryType()) {
		address = _debugger->GetAbsoluteAddress(address);
	}

	if(address.Address >= 0) {
		int64_t key = GetLabelKey(address.Address, address.Type);

		if(key >= 0) {
			auto result = _codeLabels.find(key);
			if(result != _codeLabels.end()) {
				labelInfo = result->second;
				return true;
			}
		}
	}
	return false;
}

bool LabelManager::ContainsLabel(string &label)
{
	return _codeLabelReverseLookup.find(label) != _codeLabelReverseLookup.end();
}

int32_t LabelManager::GetLabelRelativeAddress(string &label, CpuType cpuType)
{
	auto result = _codeLabelReverseLookup.find(label);
	if(result != _codeLabelReverseLookup.end()) {
		uint64_t key = result->second;
		SnesMemoryType type = GetKeyMemoryType(key);
		AddressInfo addr { (int32_t)(key & 0xFFFFFFFF), type };
		if(type <= DebugUtilities::GetLastCpuMemoryType()) {
			return addr.Address;
		}
		return _debugger->GetRelativeAddress(addr, cpuType).Address;
	}
	//Label doesn't exist
	return -2;
}

bool LabelManager::HasLabelOrComment(AddressInfo address)
{
	if(address.Type <= DebugUtilities::GetLastCpuMemoryType()) {
		address = _debugger->GetAbsoluteAddress(address);
	}

	if(address.Address >= 0) {
		uint64_t key = GetLabelKey(address.Address, address.Type);
		if(key >= 0) {
			return _codeLabels.find(key) != _codeLabels.end();
		}
	}
	return false;
}
