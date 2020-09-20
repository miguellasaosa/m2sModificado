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

#ifndef ARCH_X86_TIMING_REGISTER_FILE_H
#define ARCH_X86_TIMING_REGISTER_FILE_H

#include <lib/cpp/Debug.h>
#include <lib/cpp/IniFile.h>
#include <lib/esim/Engine.h>
#include <arch/x86/emulator/Uinst.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "BranchPredictor.h"
//#include "Cpu.h"
//#include "TraceCache.h"
#include "Uop.h"
//#include "Timing.h"



namespace x86
{

// Forward declaration
class Thread;
class Core;

class Resultados{
	public:
		struct Flips_and_time{
			int flips[32];
			int flips_flags[5];
			long long timeBit0_flags[5];
			long long timeBit0[32];
			char value[32]; //valor actual
			char value_flags[5];
			long long lastCycle[32]; 
			long long lastCycle_flags[32]; 

		};
		
		std::vector<Flips_and_time> resultados;
		
		void resul(int n){
			Flips_and_time aux;
			for(int i=0; i<32; i++){
				aux.flips[i]=0;
				aux.timeBit0[i]=0;
				aux.value[i]=0;
				aux.lastCycle[i]=0;
			}

			for(int i=0; i<5; i++){
				aux.flips_flags[i]=0;
				aux.timeBit0_flags[i]=0;
				aux.value_flags[i]=0;
				aux.lastCycle_flags[i]=0;
			}

			for(int i=0; i<n; i++){
				resultados.push_back(aux);
			}
		}

		void addFlip(int registro, int celda){
			resultados[registro].flips[celda]++;
		}


		void addFlip_flags(int registro, int celda){
			resultados[registro].flips_flags[celda]++;
		}

		void setTimeBit0(int registro, int celda, long long ciclos){
			resultados[registro].timeBit0[celda]+=ciclos;
		}

		void setTimeBit0_flags(int registro, int celda, long long ciclos){
			resultados[registro].timeBit0_flags[celda]+=ciclos;
		}

		void setValue(int registro, int celda, char value){
			resultados[registro].value[celda]=value;
		}

		void setValue_flags(int registro, int celda, char value){
			resultados[registro].value_flags[celda]=value;
		}

		void setLastCycle(int registro, int celda, long long cycle){
			resultados[registro].lastCycle[celda]=cycle;
		}

		void setLastCycle_flags(int registro, int celda, long long cycle){
			resultados[registro].lastCycle_flags[celda]=cycle;
		}

		int getFlip(int registro, int celda){
			return resultados[registro].flips[celda];
		}

		int getFlip_flags(int registro, int celda){
			return resultados[registro].flips_flags[celda];
		}

		long long  getTimeBit0(int registro, int celda){
			return resultados[registro].timeBit0[celda];
		}

		long long  getTimeBit0_flags(int registro, int celda){
			return resultados[registro].timeBit0_flags[celda];
		}

		char getValue(int registro, int celda){
			return resultados[registro].value[celda];
		}

		char getValue_flags(int registro, int celda){
			return resultados[registro].value_flags[celda];
		}

		long long getLastCycle(int registro, int celda){
			return resultados[registro].lastCycle[celda];
		}

		long long getLastCycle_flags(int registro, int celda){
			return resultados[registro].lastCycle_flags[celda];
		}

		void mostrarBin(int registro){
			printf("%s\n", resultados[registro].value);
		}

		void mostrarFlips(int registro){
			for (int i=0; i<32; i++){
				printf("%d ", resultados[registro].flips[i]);
					
			}
			printf("\n");
		}


};



// class register file
class RegisterFile
{
public:

	/// Minimum size of integer register file
	static const int MinIntegerSize;

	/// Minimum size of floating-point register file
	static const int MinFloatingPointSize;

	/// Minimum size of XMM register file
	static const int MinXmmSize;

	static int ilegales;
	static int fallidas;


	static int v_0x1;
	static int v_0x0;
	static int de0x2a0xFF;
	static int de0x01FFa0xFFFF;
	static int de0x01FFFFa0xFFFFFF;
	static int de0x01FFFFFFa0x7FFFFFFF;
	static int v_0xFFFFFFFF;
	static int de0xFFFFFFFEa0xFFFFFF00;
	static int de0xFFFFFE00a0xFFFF0000;
	static int de0xFFFE0000a0xFFFFFF00;
	static int de0xFE000000a0x80000000;


	static int total_valores;

	/*static int valor0;
	static int valor1;
	static int valor2;
	static int valor3;
	static int valor4;
	static int de5a255;
	static int de256a1023;
	static int de1024a65535;
	static int de65536a16777215;
	static int de16777216a0xbf;
	static int mayores;

	static int neg_valor1;
	static int neg_valor2;
	static int neg_valor3;
	static int neg_valor4;
	static int neg_5a255;
	static int neg_256a1023;
	static int neg_1024a65535;
	static int neg_65536a16777215;
	static int neg_de16777216a0xbf;
	static int menores;*/

	/// Privacy kinds for register file
	enum Kind
	{
		KindInvalid = 0,
		KindShared,
		KindPrivate
	};

	/// String map for values of type Kind
	static misc::StringMap KindMap;
	/*static struct Resultados{
		int flips[32];
		long long timeBit0[32];
	}*/
		
	//Resultados
	/*static struct PhysicalRegisterAux{
		int flips[32];
		long long timeBit0[32];
	};*/


	static Resultados resultados;


private:

	//
	// Class members
	//

	// Core that the register file belongs to, initialized in constructor
	Core *core;

	// Thread that the register file belongs to, initialized in constructor
	Thread *thread;

	// Structure of physical register
	struct PhysicalRegister
	{
		// Flag indicating whether the result of this physical register
		// is still being computed.
		bool pending = false;

		// Number of logical registers mapped to this physical register
		int busy = 0;

		char value[32];
		long long lastCycle[32];
		//int flips[32];
		//long long timeBit0[32];
	};



	//
	// Integer registers
	//

	// Integer register aliasing table
	int integer_rat[Uinst::DepIntCount];

	// Integer physical registers
	std::unique_ptr<PhysicalRegister[]> integer_registers;



	// List of free integer physical registers
	std::unique_ptr<int[]> free_integer_registers;

	// Number of free integer physical registers
	int num_free_integer_registers = 0;

	// Number of reads to the integer RAT
	long long num_integer_rat_reads = 0;

	// Number of writes to the integer RAT
	long long num_integer_rat_writes = 0;

	// Number of occupied integer registers
	int num_occupied_integer_registers = 0;

	// Request an integer physical register, and return its identifier
	int RequestIntegerRegister();




	//
	// Floating-point registers
	//

	// Floating-point register aliasing table
	int floating_point_rat[Uinst::DepFpCount];

	// Floating-point physical registers
	std::unique_ptr<PhysicalRegister[]> floating_point_registers;

	// List of free floating-point physical registers
	std::unique_ptr<int[]> free_floating_point_registers;

	// Number of free floating-point physical registers
	int num_free_floating_point_registers = 0;

	// Value between 0 and 7 indicating the top of the stack in the
	// floating-point register stack
	int floating_point_top = 0;

	// Number of reads to the floating-point RAT
	long long num_floating_point_rat_reads = 0;

	// Number of writes to the floating-point RAT
	long long num_floating_point_rat_writes = 0;

	// Number of occupied float point registers
	int num_occupied_floating_point_registers= 0;

	// Request a floating-point physical register, and return its
	// identifier
	int RequestFloatingPointRegister();




	//
	// XMM registers
	//
	
	// XMM register aliasing table
	int xmm_rat[Uinst::DepXmmCount];

	// XMM physical registers
	std::unique_ptr<PhysicalRegister[]> xmm_registers;

	// List of free XMM physical registers
	std::unique_ptr<int[]> free_xmm_registers;

	// Number of free XMM physical registers
	int num_free_xmm_registers = 0;

	// Number of reads to the XMM RAT
	long long num_xmm_rat_reads = 0;

	// Number of writes to the XMM RAT
	long long num_xmm_rat_writes = 0;

	// Number of occupied XMM registers
	int num_occupied_xmm_registers= 0;

	// Request an XMM physical register and return its identifier
	int RequestXmmRegister();




	//
	// Configuration
	//

	// Private/shared register file
	static Kind kind;

	// Total size of integer register file
	static int integer_size;

	// Total size of floating-point register file
	static int floating_point_size;

	// Total size of XMM register file
	static int xmm_size;



	// Per-thread size of floating-point register file
	static int floating_point_local_size;

	// Per-thread size of XMM register file
	static int xmm_local_size;

public:

	//
	// Class Error
	//

	/// Exception for X86 register file
	class Error : public misc::Error
	{
	public:

		Error(const std::string &message) : misc::Error(message)
		{
			AppendPrefix("X86 register file");
		}
	};

		// Per-thread size of integer register file
	static int integer_local_size;

	//
	// Static members
	//

	// File to dump debug information
	static std::string debug_file;

	// Debug information
	static misc::Debug debug;
	
	/// Read register file configuration from configuration file
	static void ParseConfiguration(misc::IniFile *ini_file);

	/// Return the register file kind, as configured by the user
	static Kind getKind() { return kind; }

	/// Return the integer register file size, as configured by the user.
	static int getIntegerSize() { return integer_size; }

	/// Return the floating-point register file size, as configured.
	static int getFloatingPointSize() { return floating_point_size; }

	/// Return the XMM register file size, as configured by the user.
	static int getXmmSize() { return xmm_size; }

	//static int getFlip(int physical_register, int celda) {return RegisterFile::resultados[physical_register].flips[celda];}

	//static int getTimeBit0(int physical_register, int celda){return RegisterFile::resultados[physical_register].timeBit0[celda];}




	//
	// Class members
	//

	/// Constructor
	RegisterFile(Thread *thread);

	/// Dump a plain-text representation of the object into the given output
	/// stream, or into the standard output if argument \a os is committed.
	void Dump(std::ostream &os = std::cout) const;
	
	/// Same as Dump()
	friend std::ostream &operator<<(std::ostream &os,
			const RegisterFile &register_file)
	{
		register_file.Dump(os);
		return os;
	}

	/// Return true if there are enough available physical registers to
	/// rename the given uop.
	bool canRename(Uop *uop);

	/// Perform register renaming on the given uop. This operation renames
	/// source and destination registers, requesting as many physical
	/// registers as needed for the uop.
	void Rename(Uop *uop);

	/// Check if input dependencies are resolved
	bool isUopReady(Uop *uop);

	//Añadidas por mi
	void IntToBin(unsigned numero, char binario[], int n);
	void setEstadisticas(unsigned valor);
	int actulizarRegistro(int physical_register, unsigned value, int numBytes, long long cycle, Uinst* inst);
	void actulizarRegistro_flags(int physical_register, unsigned value, long long cycle);

	/// Update the state of the register file when an uop completes, that
	/// is, when its results are written back.
	void WriteUop(Uop *uop, Thread *thread);

	/// Update the state of the register file when an uop is recovered from
	/// speculative execution
	void UndoUop(Uop *uop);

	/// Update the state of the register file when an uop commits
	void CommitUop(Uop *uop);

	/// Check integrity of register file
	void CheckRegisterFile();

	/// Check if integer register at certain index is free.
	/// Used only in testing.
	bool isIntegerRegisterFree(int index)
	{
		if (integer_registers[index].busy > 0)
			return false;
		return true;
	}

	/// Check if floating point register at certain index is free
	/// Used only in testing.
	bool isFloatingPointRegisterFree(int index)
	{
		if (floating_point_registers[index].busy > 0)
			return false;
		return true;
	}

	/// Check if xmm register at certain index is free
	/// Used only in testing
	bool isXmmRegisterFree(int index)
	{
		if (xmm_registers[index].busy > 0)
			return false;
		return true;
	}




	//
	// Statistics
	//

	/// Return the number of reads to the integer RAT
	long long getNumIntegerRatReads() const { return num_integer_rat_reads; }

	/// Return the number of writes to the integer RAT
	long long getNumIntegerRatWrites() const { return num_integer_rat_writes; }

	/// Return the number of reads to the floating-point RAT
	long long getNumFloatingPointRatReads() const { return num_floating_point_rat_reads; }

	/// Return the number of writes to the floating-point RAT
	long long getNumFloatingPointRatWrites() const { return num_floating_point_rat_writes; }

	/// Return the number of reads to the XMM RAT
	long long getNumXmmRatReads() const { return num_xmm_rat_reads; }

	/// Return the number of writes to the XMM RAT
	long long getNumXmmRatWrites() const { return num_xmm_rat_writes; }
};

}

#endif

