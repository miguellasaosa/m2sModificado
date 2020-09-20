/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "RegisterFile.h"
#include "Core.h"
#include "Thread.h"
#include "Cpu.h"
//#include <boost/assign/list_of.hpp>
//#include <boost/assign/std/vector.hpp>
//#include <boost/assert.hpp>

namespace x86
{

const int RegisterFile::MinIntegerSize = Uinst::DepIntCount + Uinst::MaxODeps;
const int RegisterFile::MinFloatingPointSize = Uinst::DepFpCount + Uinst::MaxODeps;
const int RegisterFile::MinXmmSize = Uinst::DepXmmCount + Uinst::MaxODeps;

misc::StringMap RegisterFile::KindMap =
{
	{"Shared", KindShared},
	{"Private", KindPrivate}
};

RegisterFile::Kind RegisterFile::kind;
int RegisterFile::integer_size;
int RegisterFile::floating_point_size;
int RegisterFile::xmm_size;
int RegisterFile::integer_local_size;
int RegisterFile::floating_point_local_size;
int RegisterFile::xmm_local_size;

Resultados RegisterFile::resultados;

int RegisterFile::ilegales=0;
int RegisterFile::fallidas=0;

int RegisterFile::v_0x0=0;
int RegisterFile::v_0x1=0;
int RegisterFile::de0x2a0xFF=0;
int RegisterFile::de0x01FFa0xFFFF=0;
int RegisterFile::de0x01FFFFa0xFFFFFF=0;
int RegisterFile::de0x01FFFFFFa0x7FFFFFFF=0;
int RegisterFile::v_0xFFFFFFFF=0;
int RegisterFile::de0xFFFFFFFEa0xFFFFFF00=0;
int RegisterFile::de0xFFFFFE00a0xFFFF0000=0;
int RegisterFile::de0xFFFE0000a0xFFFFFF00=0;
int RegisterFile::de0xFE000000a0x80000000=0;


int RegisterFile::total_valores=0;
/*int RegisterFile::valor0=0;
int RegisterFile::valor1=0;
int RegisterFile::valor2=0;
int RegisterFile::valor3=0;
int RegisterFile::valor4=0;
int RegisterFile::de5a255=0;
int RegisterFile::de256a1023=0;
int RegisterFile::de1024a65535=0;
int RegisterFile::de65536a16777215=0;
int RegisterFile::de16777216a0xbf=0;
int RegisterFile::mayores=0;

int RegisterFile::neg_valor1=0;
int RegisterFile::neg_valor2=0;
int RegisterFile::neg_valor3=0;
int RegisterFile::neg_valor4=0;
int RegisterFile::neg_5a255=0;
int RegisterFile::neg_256a1023=0;
int RegisterFile::neg_1024a65535=0;
int RegisterFile::neg_65536a16777215=0;
int RegisterFile::neg_de16777216a0xbf=0;
int RegisterFile::menores=0;*/

std::string RegisterFile::debug_file;
misc::Debug RegisterFile::debug;


RegisterFile::RegisterFile(Thread *thread) :
		thread(thread)
{
	// Core that the register file belongs to
	core = thread->getCore();

	long long cycles=thread->getCore()->getCpu()->getCycle();

	// Integer register file
	integer_registers = misc::new_unique_array<PhysicalRegister>(integer_local_size);


	//b /home/mlasaosa/tfg/multi2sim-5.0/src/arch/x86/timing/RegisterFile.cc:674
	/*PhysicalRegisterAux aux;

	for (int i=0; i<32; i++){
		aux.flips[i]=0;
		aux.timeBit0[i]=cycles;
	}*/
	//inicializacion de las estructuras necesarias
	RegisterFile::resultados.resul(integer_local_size);

	for (int physical_register=0; physical_register<integer_local_size; physical_register++){
		//resultados.push_back(aux);
		for (int i=0; i<32; i++){
			integer_registers[physical_register].value[i]=0;
			integer_registers[physical_register].lastCycle[i]=cycles;
		}
	}

	// Free list
	num_free_integer_registers = integer_local_size;
	free_integer_registers = misc::new_unique_array<int>(integer_local_size);
	for (int physical_register = 0;
			physical_register < integer_local_size;
			physical_register++)
		free_integer_registers[physical_register] = physical_register;

	// Floating-point register file
	floating_point_registers = misc::new_unique_array<PhysicalRegister>(floating_point_local_size);

	// Free list
	num_free_floating_point_registers = floating_point_local_size;
	free_floating_point_registers = misc::new_unique_array<int>(floating_point_local_size);

	// Initialize free list
	for (int physical_register = 0;
			physical_register < floating_point_local_size;
			physical_register++)
		free_floating_point_registers[physical_register] = physical_register;

	// XMM register file
	xmm_registers = misc::new_unique_array<PhysicalRegister>(xmm_local_size);

	// Free list
	num_free_xmm_registers = xmm_local_size;
	free_xmm_registers = misc::new_unique_array<int>(xmm_local_size);

	// Initialize free list
	for (int physical_register = 0;
			physical_register < xmm_local_size;
			physical_register++)
		free_xmm_registers[physical_register] = physical_register;

	// Initial mappings for the integer register file. Map each logical
	// register to a new physical register, and map all flags to the first
	// allocated physical register.
	int flag_physical_register = -1;
	for (int dep = 0; dep < Uinst::DepIntCount; dep++)
	{
		int physical_register;
		int dependency = dep + Uinst::DepIntFirst;
		if (Uinst::isFlagDependency(dependency))
		{
			assert(flag_physical_register >= 0);
			physical_register = flag_physical_register;
		}
		else
		{
			physical_register = RequestIntegerRegister();
			flag_physical_register = physical_register;
		}
		integer_registers[physical_register].busy++;
		integer_rat[dep] = physical_register;
	}

	// Initial mapping for floating-point registers.
	for (int dep = 0; dep < Uinst::DepFpCount; dep++)
	{
		int physical_register = RequestFloatingPointRegister();
		floating_point_registers[physical_register].busy++;
		floating_point_rat[dep] = physical_register;
	}

	// Initial mapping for xmm registers.
	for (int dep = 0; dep < Uinst::DepXmmCount; dep++)
	{
		int physical_register = RequestXmmRegister();
		xmm_registers[physical_register].busy++;
		xmm_rat[dep] = physical_register;
	}
}


void RegisterFile::Dump(std::ostream &os) const
{
	// Title
	os << "Register file\n";
	os << "-------------\n\n";

	// Integer registers
	os << "Integer registers:\n";
	os << misc::fmt("\t%d occupied, %d free, %d total\n",
			integer_local_size - num_free_integer_registers,
			num_free_integer_registers,
			integer_local_size);
	
	// Integer mappings
	os << "\tMappings:\n";
	for (int i = 0; i < Uinst::DepIntCount; i++)
	{
		Uinst::Dep dep = (Uinst::Dep) (Uinst::DepIntFirst + i);
		os << misc::fmt("\t\t%-10s -> %d\n",
				Uinst::getDependencyName(dep),
				integer_rat[i]);
	}
	
	// Integer free registers
	os << "\tFree registers: { ";
	for (int i = 0; i < num_free_integer_registers; i++)
		os << misc::fmt("%d ", free_integer_registers[i]);
	os << "}\n";
	os << '\n';

	// Floating-point registers
	os << "Floating-point registers:\n";
	os << misc::fmt("\t%d occupied, %d free, %d total\n",
			floating_point_local_size -
			num_free_floating_point_registers,
			num_free_floating_point_registers,
			floating_point_local_size);
	
	// Floating-point mappings
	os << "\tMappings:\n";
	for (int i = 0; i < Uinst::DepFpCount; i++)
	{
		Uinst::Dep dep = (Uinst::Dep) (Uinst::DepFpFirst + i);
		os << misc::fmt("\t\t%-10s -> %d\n",
				Uinst::getDependencyName(dep),
				floating_point_rat[i]);
	}
	
	// Floating-point free registers
	os << "\tFree registers: { ";
	for (int i = 0; i < num_free_floating_point_registers; i++)
		os << misc::fmt("%d ", free_floating_point_registers[i]);
	os << "}\n";
	os << '\n';
	
	// XMM registers
	os << "XMM registers:\n";
	os << misc::fmt("\t%d occupied, %d free, %d total\n",
			xmm_local_size - num_free_xmm_registers,
			num_free_xmm_registers,
			xmm_local_size);
	
	// XMM mappings
	os << "\tMappings:\n";
	for (int i = 0; i < Uinst::DepXmmCount; i++)
	{
		Uinst::Dep dep = (Uinst::Dep) (Uinst::DepXmmFirst + i);
		os << misc::fmt("\t\t%-10s -> %d\n",
				Uinst::getDependencyName(dep),
				xmm_rat[i]);
	}
	
	// XMM free registers
	os << "\tFree registers: { ";
	for (int i = 0; i < num_free_xmm_registers; i++)
		os << misc::fmt("%d ", free_xmm_registers[i]);
	os << "}\n";
	os << '\n';

}


void RegisterFile::ParseConfiguration(misc::IniFile *ini_file)
{
	// Section [Queues]
	std::string section = "Queues";

	// Read parameters
	kind = (Kind) ini_file->ReadEnum(section, "RfKind",
			KindMap, KindPrivate);
	integer_size = ini_file->ReadInt(section, "RfIntSize", 180);
	floating_point_size = ini_file->ReadInt(section, "RfFpSize", 40);
	xmm_size = ini_file->ReadInt(section, "RfXmmSize", 40);

	// Get number of threads
	int num_threads = ini_file->ReadInt("General", "Threads", 1);

	// Check valid sizes
	if (integer_size < MinIntegerSize)
		throw Error(misc::fmt("rf_int_size must be at least %d", MinIntegerSize));
	if (floating_point_size < MinFloatingPointSize)
		throw Error(misc::fmt("rf_fp_size must be at least %d", MinFloatingPointSize));
	if (xmm_size < MinXmmSize)
		throw Error(misc::fmt("rf_xmm_size must be at least %d", MinXmmSize));

	// Calculate local sizes based on private/shared register file
	if (kind == KindPrivate)
	{
		integer_local_size = integer_size;
		floating_point_local_size = floating_point_size;
		xmm_local_size = xmm_size;
	}
	else
	{
		integer_local_size = integer_size * num_threads;
		floating_point_local_size = floating_point_size * num_threads;
		xmm_local_size = xmm_size * num_threads;
	}
}


int RegisterFile::RequestIntegerRegister()
{
	// Obtain a register from the free list
	assert(num_free_integer_registers > 0);
	//ocupa registro
	int physical_register = free_integer_registers[num_free_integer_registers - 1];
	//printf("Indice (ocupar): %d\n",num_free_integer_registers-1);
	//printf("Se ocupa el registro fisico: %d\n",physical_register);
	/*if (physical_register>=166 && physical_register<=171){
		printf("Registro: %d", physical_register);
	}*/
	num_free_integer_registers--;
	core->incNumOccupiedIntegerRegisters();
	num_occupied_integer_registers++;
	assert(!integer_registers[physical_register].busy);
	assert(!integer_registers[physical_register].pending);

	// Debug
	debug << misc::fmt("  Integer register %d allocated, %d available\n",
			physical_register, num_free_integer_registers);

	// Return allocated register
	return physical_register;
}


int RegisterFile::RequestFloatingPointRegister()
{
	// Obtain a register from the free list
	assert(num_free_floating_point_registers > 0);
	int physical_register = free_floating_point_registers[num_free_floating_point_registers - 1];
	num_free_floating_point_registers--;
	core->incNumOccupiedFloatingPointRegisters();
	num_occupied_floating_point_registers++;
	assert(!floating_point_registers[physical_register].busy);
	assert(!floating_point_registers[physical_register].pending);

	// Debug
	debug << misc::fmt("  Floating-point register %d allocated, "
			"%d available\n",
			physical_register,
			num_free_floating_point_registers);

	// Return allocated register
	return physical_register;
}


int RegisterFile::RequestXmmRegister()
{
	// Obtain a register from the free list
	assert(num_free_xmm_registers > 0);
	int physical_register = free_xmm_registers[num_free_xmm_registers - 1];
	num_free_xmm_registers--;
	core->incNumOccupiedXmmRegisters();
	num_occupied_xmm_registers++;
	assert(!xmm_registers[physical_register].busy);
	assert(!xmm_registers[physical_register].pending);

	// Debug
	debug << misc::fmt("  XMM register %d allocated, %d available\n",
			physical_register,
			num_free_xmm_registers);

	// Return allocated register
	return physical_register;
}


bool RegisterFile::canRename(Uop *uop)
{
	// Detect negative cases
	if (kind == KindPrivate)
	{
		// Not enough integer registers
		if (num_occupied_integer_registers
				+ uop->getNumIntegerOutputs()
				> integer_local_size)
			return false;

		// Not enough floating-point registers
		if (num_occupied_floating_point_registers
				+ uop->getNumFloatingPointOutputs()
				> floating_point_local_size)
			return false;

		// Not enough XMM registers
		if (num_occupied_xmm_registers
				+ uop->getNumXmmOutputs()
				> xmm_local_size)
			return false;
	}
	else
	{
		// Not enough integer registers
		if (core->getNumOccupiedIntegerRegisters()
				+ uop->getNumIntegerOutputs()
				> integer_local_size)
			return false;

		// Not enough floating-point registers
		if (core->getNumOccupiedFloatingPointRegisters()
				+ uop->getNumFloatingPointOutputs()
				> floating_point_local_size)
			return false;

		// Not enough XMM registers
		if (core->getNumOccupiedXmmRegisters()
				+ uop->getNumXmmOutputs()
				> xmm_local_size)
			return false;
	}

	// Uop can be renamed
	return true;
}


void RegisterFile::Rename(Uop *uop)
{

	// Update floating-point top of stack
	if (uop->getOpcode() == Uinst::OpcodeFpPop)
	{
		// Pop floating-point stack
		floating_point_top = (floating_point_top + 1) % 8;

		// Debug
		debug << misc::fmt("  Floating-point stack popped, top = %d\n",
				floating_point_top);
	}
	else if (uop->getOpcode() == Uinst::OpcodeFpPush)
	{
		// Push floating-point stack
		floating_point_top = (floating_point_top + 7) % 8;

		// Debug
		debug << misc::fmt("  Floating-point stack pushed, top = %d\n",
				floating_point_top);
	}

	// Debug
	debug << "Rename uop " << *uop << '\n';

	// Rename input int/FP/XMM registers
	for (int dep = 0; dep < Uinst::MaxIDeps; dep++)
	{
		int logical_register = uop->getUinst()->getIDep(dep);
		if (Uinst::isIntegerDependency(logical_register))
		{
			// Rename register
			int physical_register = integer_rat[logical_register - Uinst::DepIntFirst];
			uop->setInput(dep, physical_register);

			// Debug
			debug << "  Input " << Uinst::dep_map[logical_register]
					<< " -> Integer regsiter "
					<< physical_register << '\n';

			// Stats
			num_integer_rat_reads++;
		}
		else if (Uinst::isFloatingPointDependency(logical_register))
		{
			// Convert to top-of-stack relative
			int stack_register = (logical_register
					- Uinst::DepFpFirst + floating_point_top)
					% 8 + Uinst::DepFpFirst;
			assert(stack_register >= Uinst::DepFpFirst
					&& stack_register <= Uinst::DepFpLast);

			// Rename register
			int physical_register = floating_point_rat[stack_register - Uinst::DepFpFirst];
			uop->setInput(dep, physical_register);

			// Debug
			debug << "  Input " << Uinst::dep_map[logical_register]
					<< " -> Floating-point regsiter "
					<< physical_register << '\n';

			// Stats
			num_floating_point_rat_reads++;
		}
		else if (Uinst::isXmmDependency(logical_register))
		{
			// Rename register
			int physical_register = xmm_rat[logical_register - Uinst::DepXmmFirst];
			uop->setInput(dep, physical_register);

			// Debug
			debug << "  Input " << Uinst::dep_map[logical_register]
					<< " -> XMM regsiter "
					<< physical_register << '\n';

			// Stats
			num_xmm_rat_reads++;
		}
		else
		{
			uop->setInput(dep, -1);
		}
	}

	// Rename output int/FP/XMM registers (not flags)
	int flag_physical_register = -1;
	int flag_count = 0;
	for (int dep = 0; dep < Uinst::MaxODeps; dep++)
	{
		int logical_register = uop->getUinst()->getODep(dep);
		if (Uinst::isFlagDependency(logical_register))
		{
			// Record a new flag
			flag_count++;
		}
		else if (Uinst::isIntegerDependency(logical_register))
		{
			if(uop->getUinst()->numBytes != -1) {
				Context::uinstModificadas++;
				Context::uinstTotales++;
			}else{
				Context::uinstTotales++;
			}
			// Request a free integer register
			int physical_register = RequestIntegerRegister();
			integer_registers[physical_register].busy++;
			integer_registers[physical_register].pending = true;
			int old_physical_register = integer_rat[logical_register - Uinst::DepIntFirst];
			if (flag_physical_register < 0)
				flag_physical_register = physical_register;

			// Allocate it
			uop->setOutput(dep, physical_register);
			uop->setOldOutput(dep, old_physical_register);
			integer_rat[logical_register - Uinst::DepIntFirst] = physical_register;

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> Integer register "
					<< physical_register << '\n';

			// Stats
			num_integer_rat_writes++;
		}
		else if (Uinst::isFloatingPointDependency(logical_register))
		{
			// Convert to top-of-stack relative
			int stack_register = (logical_register
					- Uinst::DepFpFirst + floating_point_top)
					% 8 + Uinst::DepFpFirst;
			assert(stack_register >= Uinst::DepFpFirst
					&& stack_register <= Uinst::DepFpLast);

			// Request a free floating-point register
			int physical_register = RequestFloatingPointRegister();
			floating_point_registers[physical_register].busy++;
			floating_point_registers[physical_register].pending = true;
			int old_physical_register = floating_point_rat[stack_register - Uinst::DepFpFirst];

			// Allocate it
			uop->setOutput(dep, physical_register);
			uop->setOldOutput(dep, old_physical_register);
			floating_point_rat[stack_register - Uinst::DepFpFirst] = physical_register;

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> Floating-point register "
					<< physical_register << '\n';

			// Stats
			num_floating_point_rat_writes++;
		}
		else if (Uinst::isXmmDependency(logical_register))
		{
			// Request a free XMM register
			int physical_register = RequestXmmRegister();
			xmm_registers[physical_register].busy++;
			xmm_registers[physical_register].pending = true;
			int old_physical_register = xmm_rat[logical_register - Uinst::DepXmmFirst];

			// Allocate it
			uop->setOutput(dep, physical_register);
			uop->setOldOutput(dep, old_physical_register);
			xmm_rat[logical_register - Uinst::DepXmmFirst] = physical_register;

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> XMM register "
					<< physical_register << '\n';

			// Stats
			num_xmm_rat_writes++;
		}
		else
		{
			// Not a valid output dependence
			uop->setOutput(dep, -1);
			uop->setOldOutput(dep, -1);
		}
	}

	// Rename flags
	if (flag_count > 0)
	{
		// Request flag register
		if (flag_physical_register < 0)
			flag_physical_register = RequestIntegerRegister();

		// Traverse dependencies
		for (int dep = 0; dep < Uinst::MaxODeps; dep++)
		{
			// Ignore if not a flag
			int logical_register = uop->getUinst()->getODep(dep);
			if (!Uinst::isFlagDependency(logical_register))
				continue;

			// Rename
			integer_registers[flag_physical_register].busy++;
			integer_registers[flag_physical_register].pending = true;
			int old_physical_register = integer_rat[logical_register - Uinst::DepIntFirst];
			uop->setOutput(dep, flag_physical_register);
			uop->setOldOutput(dep, old_physical_register);
			integer_rat[logical_register - Uinst::DepIntFirst] = flag_physical_register;

			// Debug
			debug << "  Output flag " << Uinst::dep_map[logical_register]
					<< " -> Integer register "
					<< flag_physical_register << '\n';

		}
	}
}


bool RegisterFile::isUopReady(Uop *uop)
{
	// If uop is marked as ready, it means that we verified that it is
	// ready before. The uop ready state can never change from true to
	// false.
	if (uop->ready)
		return true;

	// Traverse dependencies
	for (int dep = 0; dep < Uinst::MaxIDeps; dep++)
	{
		// Get dependencies
		int logical_register = uop->getUinst()->getIDep(dep);
		int physical_register = uop->getInput(dep);

		// Integer dependency
		if (Uinst::isIntegerDependency(logical_register)
				&& integer_registers[physical_register].pending)
			return false;

		// Floating-point dependency
		if (Uinst::isFloatingPointDependency(logical_register)
				&& floating_point_registers[physical_register].pending)
			return false;

		// XMM dependency
		if (Uinst::isXmmDependency(logical_register)
				&& xmm_registers[physical_register].pending)
			return false;
	}

	// At this point, we found that the uop is ready. Save this information
	// in the 'ready' field of the uop to avoid having to check in the
	// future.
	uop->ready = true;
	return true;
}

/*void RegisterFile::IntToBin(unsigned numero, char binario[], int n){
	int i=0;
	if (numero > 0) {
	    while (numero > 0 && i<n) {
	        if (numero%2 == 0) {
	            binario[i]=0;
	        } else {
	            binario[i]=1;
	        }
	        numero = (unsigned) numero/2;
	        i++;
	    }
	}else if (numero == 0) {
	    binario[i]=0;
	    i=1;
	}
    for(int j=i; j<n; j++) {
		binario[j]=0;
    }
}*/


void RegisterFile::IntToBin(unsigned numero, char binario[], int n){
	int i=0;

	for (i = 0; i< n; i++){
		unsigned aux=numero&1;
		if (aux == 1){
			binario[i]='1';
		}else{
			binario[i]='0';
		}
	  	numero >>=1;
	}
	binario[n]='\0';

}

  
void toBinary(unsigned n, char buffer[])
{
    int i=0;
    while(n!=0) {buffer[i]=(n%2==0 ?'0':'1'); n/=2; i++;}
}

void RegisterFile::setEstadisticas(unsigned valor){
	if(valor==0) v_0x0++;
	if(valor==0x1) v_0x1++;
	else if(valor>=0x2 && valor<=0xFF ) de0x2a0xFF++;
	else if(valor>=0x01FF && valor<=0xFFFF) de0x01FFa0xFFFF++;
	else if(valor>=0x01FFFF && valor<=0xFFFFFF) de0x01FFFFa0xFFFFFF++;
	else if(valor>=0x01FFFFFF && valor<=0x7FFFFFFF) de0x01FFFFFFa0x7FFFFFFF++;
	else if(valor==0xFFFFFFFF) v_0xFFFFFFFF++;
	else if(valor<=0xFFFFFFFE && valor>=0xFFFFFF00) de0xFFFFFFFEa0xFFFFFF00++;
	else if(valor<=0xFFFFFE00 && valor>=0xFFFF0000) de0xFFFFFE00a0xFFFF0000++;
	else if(valor<=0xFFFE0000 && valor>=0xFF000000) de0xFFFE0000a0xFFFFFF00++;
	else if(valor<=0xFE000000 && valor>=0x80000000) de0xFE000000a0x80000000++;
	total_valores++;
}




int RegisterFile::actulizarRegistro(int physical_register, unsigned value, int numBytes, long long cycle, Uinst* inst){
	//if (value<0) return -1;//Error
	if (numBytes<-1 || numBytes>5) return -1;
	if (numBytes<=0) numBytes=0;
	char binario[32];

	long long lastCycleI;
	int bitIni,bitFin;
	if (numBytes==5){
		bitIni=8;
		bitFin=16;
		value<<=8;
	}else{
		bitIni=0;
		bitFin=numBytes*8;
	}

	IntToBin(value,binario,32);

	
	//printf("Rango: %d - %d\n",bitIni, bitFin);
	//printf("Vector flips: ");
	//RegisterFile::resultados.mostrarFlips(physical_register);
	//printf("Binario: %s\n", binario);
	//printf("Decimal: %u\n", value);


	/*if (physical_register>=166 && physical_register <= 171){
		printf("Se deberia modificar el registro %d", physical_register);
	}*/

	setEstadisticas(value);

	for(int i=bitIni; i<bitFin; i++){


		if(binario[i] != RegisterFile::resultados.getValue(physical_register, i)){
			RegisterFile::resultados.addFlip(physical_register,i);
			
			/*if(physical_register==3){
				RegisterFile::resultados.mostrarBin(physical_register);
				printf("Flip %d en el bit 30:  %s\n\n", RegisterFile::resultados.getFlip(physical_register,30), binario);
			}*/

			if(binario[i]=='1'){
				//si se va a escribir un uno y en la celda antes habia un 0;
				lastCycleI=RegisterFile::resultados.getLastCycle(physical_register,i);
				lastCycleI=cycle-lastCycleI;
				RegisterFile::resultados.setTimeBit0(physical_register,i,lastCycleI);
			}
			else{
				RegisterFile::resultados.setLastCycle(physical_register,i,cycle);
			}
		}	
		RegisterFile::resultados.setValue(physical_register,i,binario[i]);
	}
	return 0;
}

void RegisterFile::actulizarRegistro_flags(int physical_register, unsigned value, long long cycle){
	char binario[5];
	IntToBin(value,binario,5);

	//printf("Se deberia modificar el registro %d\n", physical_register);
	for(int i=0; i<5; i++){
		if(binario[i] != RegisterFile::resultados.getValue_flags(physical_register, i)){
			RegisterFile::resultados.addFlip_flags(physical_register,i);
			
			if(binario[i]==1){
				//si se va a escribir un uno y en la celda antes habia un 0;
				long long lastCycleI=RegisterFile::resultados.getLastCycle_flags(physical_register,i);
				lastCycleI=cycle-lastCycleI;
				RegisterFile::resultados.setTimeBit0_flags(physical_register,i,lastCycleI);
			}
			else{
				RegisterFile::resultados.setLastCycle_flags(physical_register,i,cycle);
			}
		}	
		RegisterFile::resultados.setValue_flags(physical_register,i,binario[i]);
	}
}


void RegisterFile::WriteUop(Uop *uop, Thread *thread)
{
	long long cycles=thread->getCore()->getCpu()->getCycle();
	int esFallida=0;
	//esim::Engine *esim_engine = esim::Engine::getInstance();
	int segundoValor=0;
	for (int dep = 0; dep < Uinst::MaxODeps; dep++)
	{
		int logical_register = uop->getUinst()->getODep(dep);
		int physical_register = uop->getOutput(dep);
		if (Uinst::isIntegerDependency(logical_register)){
			integer_registers[physical_register].pending = false;
			if (!Uinst::isFlagDependency(logical_register)){
				if (segundoValor==0){
					esFallida=actulizarRegistro(physical_register,uop->getUinst()->valor, uop->getUinst()->numBytes, cycles, uop->getUinst());
					if(esFallida==-1) RegisterFile::fallidas++;
					segundoValor++;
				}
				else {
					esFallida=actulizarRegistro(physical_register,uop->getUinst()->valor2, uop->getUinst()->numBytes2, cycles, uop->getUinst());
					if(esFallida==-1) RegisterFile::fallidas++;
				}
			}else{//flag dependency
				actulizarRegistro_flags(physical_register,uop->getUinst()->flags,cycles);
			}
		}
		else if (Uinst::isFloatingPointDependency(logical_register))
			floating_point_registers[physical_register].pending = false;
		else if (Uinst::isXmmDependency(logical_register))
			xmm_registers[physical_register].pending = false;
	}
}


void RegisterFile::UndoUop(Uop *uop)
{

	// Debug
	debug << "Undo uop " << *uop << '\n';

	// Undo mappings in reverse order, in case an instruction has a
	// duplicated output dependence.
	assert(uop->speculative_mode);
	for (int dep = Uinst::MaxODeps - 1; dep >= 0; dep--)
	{
		int logical_register = uop->getUinst()->getODep(dep);
		int physical_register = uop->getOutput(dep);
		int old_physical_register = uop->getOldOutput(dep);
		if (Uinst::isIntegerDependency(logical_register))
		{
			// Decrease busy counter and free if 0.
			assert(integer_registers[physical_register].busy > 0);
			assert(!integer_registers[physical_register].pending);
			integer_registers[physical_register].busy--;
			if (!integer_registers[physical_register].busy)
			{
				// Sanity
				assert(num_free_integer_registers < integer_local_size);
				assert(core->getNumOccupiedIntegerRegisters() > 0
						&& num_occupied_integer_registers > 0);

				// Add to free list
				free_integer_registers[num_free_integer_registers] = physical_register;
				num_free_integer_registers++;

				// One less register occupied
				core->decNumOccupiedIntegerRegisters();
				num_occupied_integer_registers--;

				// Debug
				debug << misc::fmt("  Integer register %d freed\n",
						physical_register);
			}

			// Return to previous mapping
			integer_rat[logical_register - Uinst::DepIntFirst] = old_physical_register;
			assert(integer_registers[old_physical_register].busy);

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> From integer register "
					<< physical_register << " back to "
					<< old_physical_register << '\n';
		}
		else if (Uinst::isFloatingPointDependency(logical_register))
		{
			// Convert to top-of-stack relative
			int stack_register = (logical_register - Uinst::DepFpFirst
					+ floating_point_top) % 8
					+ Uinst::DepFpFirst;
			assert(stack_register >= Uinst::DepFpFirst &&
					stack_register <= Uinst::DepFpLast);

			// Decrease busy counter and free if 0.
			assert(floating_point_registers[physical_register].busy > 0);
			assert(!floating_point_registers[physical_register].pending);
			floating_point_registers[physical_register].busy--;
			if (!floating_point_registers[physical_register].busy)
			{
				// Sanity
				assert(num_free_floating_point_registers < floating_point_local_size);
				assert(core->getNumOccupiedFloatingPointRegisters() > 0
						&& num_occupied_floating_point_registers > 0);

				// Add to free list
				free_floating_point_registers[num_free_floating_point_registers] = physical_register;
				num_free_floating_point_registers++;

				// One less register occupied
				core->decNumOccupiedFloatingPointRegisters();
				num_occupied_floating_point_registers--;

				// Debug
				debug << misc::fmt("  Floating-point register %d freed\n",
						physical_register);
			}

			// Return to previous mapping
			floating_point_rat[stack_register - Uinst::DepFpFirst] = old_physical_register;
			assert(floating_point_registers[old_physical_register].busy);

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> From floating-point register "
					<< physical_register << " back to "
					<< old_physical_register << '\n';
		}
		else if (Uinst::isXmmDependency(logical_register))
		{
			// Decrease busy counter and free if 0.
			assert(xmm_registers[physical_register].busy > 0);
			assert(!xmm_registers[physical_register].pending);
			xmm_registers[physical_register].busy--;
			if (!xmm_registers[physical_register].busy)
			{
				// Sanity
				assert(num_free_xmm_registers < xmm_local_size);
				assert(core->getNumOccupiedXmmRegisters() > 0
						&& num_occupied_xmm_registers > 0);

				// Add to free list
				free_xmm_registers[num_free_xmm_registers] = physical_register;
				num_free_xmm_registers++;

				// One less register occupied
				core->decNumOccupiedXmmRegisters();
				num_occupied_xmm_registers--;

				// Debug
				debug << misc::fmt("  XMM register %d freed\n",
						physical_register);
			}

			// Return to previous mapping
			xmm_rat[logical_register - Uinst::DepXmmFirst] = old_physical_register;
			assert(xmm_registers[old_physical_register].busy);

			// Debug
			debug << "  Output " << Uinst::dep_map[logical_register]
					<< " -> From XMM register "
					<< physical_register << " back to "
					<< old_physical_register << '\n';
		}
		else
		{
			// Not a valid dependence.
			assert(physical_register == -1);
			assert(old_physical_register == -1);
		}
	}

	// Undo modification in floating-point top of stack
	if (uop->getOpcode() == Uinst::OpcodeFpPop)
	{
		// Inverse-pop floating-point stack
		floating_point_top = (floating_point_top + 7) % 8;
	}
	else if (uop->getOpcode() == Uinst::OpcodeFpPush)
	{
		// Inverse-push floating-point stack
		floating_point_top = (floating_point_top + 1) % 8;
	}
}


void RegisterFile::CommitUop(Uop *uop)
{

	// Debug
	debug << "Commit uop " << *uop << '\n';

	// Traverse output dependencies
	assert(!uop->speculative_mode);
	for (int dep = 0; dep < Uinst::MaxODeps; dep++)
	{
		int logical_register = uop->getUinst()->getODep(dep);
		int physical_register = uop->getOutput(dep);
		int old_physical_register = uop->getOldOutput(dep);

		if (Uinst::isIntegerDependency(logical_register))
		{
			// Decrease counter of previous mapping and free if 0.
			assert(integer_registers[old_physical_register].busy > 0);
			integer_registers[old_physical_register].busy--;
			if (!integer_registers[old_physical_register].busy)
			{
				// Sanity
				assert(!integer_registers[old_physical_register].pending);
				assert(num_free_integer_registers < integer_local_size);
				assert(core->getNumOccupiedIntegerRegisters() > 0
						&& num_occupied_integer_registers > 0);

				// Add to free list
				//libera registro
				free_integer_registers[num_free_integer_registers] = old_physical_register;
				num_free_integer_registers++;
				//printf("Indice (liberar): %d\n",num_free_integer_registers);
				//printf("Se libera el registro fisico: %d\n",physical_register);
				// One less register occupied
				core->decNumOccupiedIntegerRegisters();
				num_occupied_integer_registers--;

				// Debug
				debug << misc::fmt("  Integer register %d freed\n",
						physical_register);
			}
		}
		else if (Uinst::isFloatingPointDependency(logical_register))
		{
			// Decrease counter of previous mapping and free if 0.
			assert(floating_point_registers[old_physical_register].busy > 0);
			floating_point_registers[old_physical_register].busy--;
			if (!floating_point_registers[old_physical_register].busy)
			{
				// Sanity
				assert(!floating_point_registers[old_physical_register].pending);
				assert(num_free_floating_point_registers < floating_point_local_size);
				assert(core->getNumOccupiedFloatingPointRegisters() > 0
						&& num_occupied_floating_point_registers > 0);

				// Add to free list
				free_floating_point_registers[num_free_floating_point_registers] = old_physical_register;
				num_free_floating_point_registers++;

				// One less register occupied
				core->decNumOccupiedFloatingPointRegisters();
				num_occupied_floating_point_registers--;

				// Debug
				debug << misc::fmt("  Floating-point register %d freed\n",
						physical_register);
			}
		}
		else if (Uinst::isXmmDependency(logical_register))
		{
			// Decrease counter of previous mapping and free if 0.
			assert(xmm_registers[old_physical_register].busy > 0);
			xmm_registers[old_physical_register].busy--;
			if (!xmm_registers[old_physical_register].busy)
			{
				// Sanity
				assert(!xmm_registers[old_physical_register].pending);
				assert(num_free_xmm_registers < xmm_local_size);
				assert(core->getNumOccupiedXmmRegisters() > 0
						&& num_occupied_xmm_registers > 0);

				// Add to free list
				free_xmm_registers[num_free_xmm_registers] = old_physical_register;
				num_free_xmm_registers++;

				// One less register occupied
				core->decNumOccupiedXmmRegisters();
				num_occupied_xmm_registers--;

				// Debug
				debug << misc::fmt("  XMM register %d freed\n",
						physical_register);
			}
		}
		else
		{
			// Not a valid dependence.
			assert(physical_register == -1);
			assert(old_physical_register == -1);
		}
	}
}


void RegisterFile::CheckRegisterFile()
{
	// Check that all registers in the free list are actually free.
	for (int i = 0; i < num_free_integer_registers; i++)
	{
		int physical_register = free_integer_registers[i];
		assert(!integer_registers[physical_register].busy);
		assert(!integer_registers[physical_register].pending);
	}
	for (int i = 0; i < num_free_floating_point_registers; i++)
	{
		int physical_register = free_floating_point_registers[i];
		assert(!floating_point_registers[physical_register].busy);
		assert(!floating_point_registers[physical_register].pending);
	}
	for (int i = 0; i < num_free_xmm_registers; i++)
	{
		int physical_register = free_xmm_registers[i];
		assert(!xmm_registers[physical_register].busy);
		assert(!xmm_registers[physical_register].pending);
	}

	// Check that all mapped integer registers are busy
	for (int logical_register = Uinst::DepIntFirst;
			logical_register <= Uinst::DepIntLast;
			logical_register++)
	{
		int physical_register = integer_rat[logical_register - Uinst::DepIntFirst];
		assert(integer_registers[physical_register].busy);
	}

	// Floating-point registers
	for (int logical_register = Uinst::DepFpFirst;
			logical_register <= Uinst::DepFpLast;
			logical_register++)
	{
		int physical_register = floating_point_rat[logical_register - Uinst::DepFpFirst];
		assert(floating_point_registers[physical_register].busy);
	}

	// XMM registers
	for (int logical_register = Uinst::DepXmmFirst;
			logical_register <= Uinst::DepXmmLast;
			logical_register++)
	{
		int physical_register = xmm_rat[logical_register - Uinst::DepXmmFirst];
		assert(xmm_registers[physical_register].busy);
	}
}


}
