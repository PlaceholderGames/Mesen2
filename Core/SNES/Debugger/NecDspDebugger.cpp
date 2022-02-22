#include "stdafx.h"
#include "SNES/BaseCartridge.h"
#include "SNES/SnesMemoryManager.h"
#include "SNES/SnesConsole.h"
#include "SNES/Coprocessors/DSP/NecDsp.h"
#include "SNES/Debugger/NecDspDebugger.h"
#include "SNES/Debugger/TraceLogger/NecDspTraceLogger.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Disassembler.h"
#include "Debugger/CallstackManager.h"
#include "Debugger/BreakpointManager.h"
#include "Debugger/Debugger.h"
#include "Debugger/MemoryAccessCounter.h"
#include "Debugger/ExpressionEvaluator.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "MemoryOperationType.h"

NecDspDebugger::NecDspDebugger(Debugger* debugger)
{
	SnesConsole* console = (SnesConsole*)debugger->GetConsole();

	_debugger = debugger;
	_disassembler = debugger->GetDisassembler();
	_dsp = console->GetCartridge()->GetDsp();
	_settings = debugger->GetEmulator()->GetSettings();
	
	_traceLogger.reset(new NecDspTraceLogger(debugger, this, console->GetPpu(), console->GetMemoryManager()));

	_breakpointManager.reset(new BreakpointManager(debugger, this, CpuType::NecDsp, debugger->GetEventManager(CpuType::Snes)));
	_step.reset(new StepRequest());
}

void NecDspDebugger::Reset()
{
}

void NecDspDebugger::ProcessInstruction()
{
	uint32_t addr = _dsp->GetState().PC * 3;
	uint16_t value = _dsp->GetOpCode(_dsp->GetState().PC);
	AddressInfo addressInfo = { (int32_t)addr, MemoryType::DspProgramRom };
	MemoryOperationInfo operation(addr, value, MemoryOperationType::ExecOpCode, MemoryType::NecDspMemory);

	if(_traceLogger->IsEnabled() || _settings->CheckDebuggerFlag(DebuggerFlags::NecDspDebuggerEnabled)) {
		_disassembler->BuildCache(addressInfo, 0, CpuType::NecDsp);

		if(_traceLogger->IsEnabled()) {
			DisassemblyInfo disInfo = _disassembler->GetDisassemblyInfo(addressInfo, addr, 0, CpuType::NecDsp);
			_traceLogger->Log(_dsp->GetState(), disInfo, operation);
		}
	}

	_prevProgramCounter = addr;
	_step->ProcessCpuExec();
	_debugger->ProcessBreakConditions(CpuType::NecDsp, *_step.get(), _breakpointManager.get(), operation, addressInfo);
}

void NecDspDebugger::ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type)
{
	//TODO
}

void NecDspDebugger::ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type)
{
	//TODO
}

void NecDspDebugger::Run()
{
	_step.reset(new StepRequest());
}

void NecDspDebugger::Step(int32_t stepCount, StepType type)
{
	StepRequest step;

	switch(type) {
		case StepType::Step: step.StepCount = stepCount; break;
		
		case StepType::StepOut:
		case StepType::StepOver:
			step.StepCount = 1;
			break;

		case StepType::SpecificScanline:
		case StepType::PpuStep:
			break;
	}

	_step.reset(new StepRequest(step));
}

CallstackManager* NecDspDebugger::GetCallstackManager()
{
	return nullptr;
}

BreakpointManager* NecDspDebugger::GetBreakpointManager()
{
	return _breakpointManager.get();
}

IAssembler* NecDspDebugger::GetAssembler()
{
	throw std::runtime_error("Assembler not supported for NEC DSP");
}

BaseEventManager* NecDspDebugger::GetEventManager()
{
	throw std::runtime_error("Event manager not supported for NEC DSP");
}

CodeDataLogger* NecDspDebugger::GetCodeDataLogger()
{
	return nullptr;
}

BaseState& NecDspDebugger::GetState()
{
	return _dsp->GetState();
}

ITraceLogger* NecDspDebugger::GetTraceLogger()
{
	return _traceLogger.get();
}
