/*
Copyright (C) 2006 - 2014 Evan Teran
                          eteran@alum.rit.edu

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ArchProcessor.h"
#include "Configuration.h"
#include "IDebuggerCore.h"
#include "Instruction.h"
#include "Prototype.h"
#include "RegisterListWidget.h"
#include "State.h"
#include "Util.h"
#include "edb.h"
#include "string_hash.h"

#include <QApplication>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QVector>
#include <QXmlQuery>

#include <boost/math/special_functions/fpclassify.hpp>
#include <climits>
#include <cmath>

#ifdef Q_OS_LINUX
#include <asm/unistd.h>
#endif

namespace {

//------------------------------------------------------------------------------
// Name: create_register_item
// Desc:
//------------------------------------------------------------------------------
QTreeWidgetItem *create_register_item(QTreeWidgetItem *parent, const QString &name) {

	QTreeWidgetItem *const item = new QTreeWidgetItem(parent);
	item->setData(0, Qt::UserRole, name);
	return item;
}

//------------------------------------------------------------------------------
// Name: get_effective_address
// Desc:
//------------------------------------------------------------------------------
edb::address_t get_effective_address(const edb::Operand &op, const State &state) {
	edb::address_t ret = 0;

	// TODO: get registers by index, not string! too slow

	if(op.valid()) {
		switch(op.general_type()) {
		case edb::Operand::TYPE_REGISTER:
			ret = state[QString::fromStdString(to_string(op))].value<edb::reg_t>();
			break;
		case edb::Operand::TYPE_EXPRESSION:
			do {

				edb::reg_t base = state[QString::fromStdString(edisassm::register_name<edisassm::x86_64>(op.expression().base))].value<edb::reg_t>();
				if(op.expression().base == edb::Operand::REG_RIP) {
					base += op.owner()->size();
				}

				const edb::reg_t index = state[QString::fromStdString(edisassm::register_name<edisassm::x86_64>(op.expression().index))].value<edb::reg_t>();
				ret                    = base + index * op.expression().scale + op.displacement();

				if(op.owner()->prefix() & edb::Instruction::PREFIX_GS) {
					ret += state["gs_base"].value<edb::reg_t>();
				}

				if(op.owner()->prefix() & edb::Instruction::PREFIX_FS) {
					ret += state["fs_base"].value<edb::reg_t>();
				}
			} while(0);
			break;
		case edb::Operand::TYPE_ABSOLUTE:
			ret = op.absolute().offset;
			if(op.owner()->prefix() & edb::Instruction::PREFIX_GS) {
				ret += state["gs_base"].value<edb::reg_t>();
			}

			if(op.owner()->prefix() & edb::Instruction::PREFIX_FS) {
				ret += state["fs_base"].value<edb::reg_t>();
			}
			break;
		case edb::Operand::TYPE_IMMEDIATE:
			break;
		case edb::Operand::TYPE_REL:
			ret = op.relative_target();
			break;
		default:
			break;
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
// Name: format_pointer
// Desc:
//------------------------------------------------------------------------------
QString format_pointer(int pointer_level, edb::reg_t arg, QChar type) {
	
	Q_UNUSED(type);
	Q_UNUSED(pointer_level);
	Q_ASSERT(pointer_level >= 1);
	
	if(arg == 0) {
		return "NULL";
	} else {
		return QString("0x%1").arg(edb::v1::format_pointer(arg));
	}
}

//------------------------------------------------------------------------------
// Name: format_integer
// Desc:
//------------------------------------------------------------------------------
QString format_integer(int pointer_level, edb::reg_t arg, QChar type) {
	if(pointer_level > 0) {
		return format_pointer(pointer_level, arg, type);
	}
	
	QString s;
	
	switch(type.toLatin1()) {
	case 'w': return s.sprintf("%u", static_cast<wchar_t>(arg));
	case 'b': return s.sprintf("%d", static_cast<bool>(arg));
	case 'c':
		if(arg < 0x80 && (std::isprint(arg) || std::isspace(arg))) {
			return s.sprintf("'%c'", static_cast<char>(arg));
		} else {
			return s.sprintf("'\\x%02x'", static_cast<quint16>(arg));
		}
	
	
	case 'a': return s.sprintf("%d", static_cast<signed char>(arg));
	case 'h': return s.sprintf("%u", static_cast<unsigned char>(arg));
	case 's': return s.sprintf("%d", static_cast<short>(arg));
	case 't': return s.sprintf("%u", static_cast<unsigned short>(arg));
	case 'i': return s.sprintf("%d", static_cast<int>(arg));
	case 'j': return s.sprintf("%u", static_cast<unsigned int>(arg));
	case 'l': return s.sprintf("%ld", static_cast<long>(arg));
	case 'm': return s.sprintf("%lu", static_cast<unsigned long>(arg));
	case 'x': return s.sprintf("%lld", static_cast<long long>(arg));
	case 'y': return s.sprintf("%llu", static_cast<long unsigned long>(arg));
	case 'n':
	case 'o':
	default:
		return format_pointer(pointer_level, arg, type);
	}
}

//------------------------------------------------------------------------------
// Name: format_integer
// Desc:
//------------------------------------------------------------------------------
QString format_char(int pointer_level, edb::reg_t arg, QChar type) {
	if(pointer_level == 1) {
		if(arg == 0) {
			return "NULL";
		} else {
			QString string_param;
			int string_length;

			if(edb::v1::get_ascii_string_at_address(arg, string_param, edb::v1::config().min_string_length, 256, string_length)) {
				return QString("<0x%1> \"%2\"").arg(edb::v1::format_pointer(arg)).arg(string_param);
			} else {
				char character;
				edb::v1::debugger_core->read_bytes(arg, &character, sizeof(character));
				if(character == '\0') {
					return QString("<0x%1> \"\"").arg(edb::v1::format_pointer(arg));
				} else {
					return QString("<0x%1>").arg(edb::v1::format_pointer(arg));
				}
			}
		}
	} else {
		return format_integer(pointer_level, arg, type);
	}
}

//------------------------------------------------------------------------------
// Name: format_argument
// Desc:
//------------------------------------------------------------------------------
QString format_argument(const QString &type, edb::reg_t arg) {
	QString param_text;
	
	int pointer_level = 0;
	Q_FOREACH(QChar ch, type) {
	
		if(ch == 'P') {
			++pointer_level;
		} else if(ch == 'r' || ch == 'V' || ch == 'K') {
			// skip things like const, volatile, restrict, they don't effect
			// display for us
			continue;
		} else {
			switch(ch.toLatin1()) {
			case 'v': return format_pointer(pointer_level, arg, ch);
			case 'w': return format_integer(pointer_level, arg, ch);
			case 'b': return format_integer(pointer_level, arg, ch);
			case 'c': return format_char(pointer_level, arg, ch);
			case 'a': return format_integer(pointer_level, arg, ch);
			case 'h': return format_integer(pointer_level, arg, ch);
			case 's': return format_integer(pointer_level, arg, ch);
			case 't': return format_integer(pointer_level, arg, ch);
			case 'i': return format_integer(pointer_level, arg, ch);
			case 'j': return format_integer(pointer_level, arg, ch);
			case 'l': return format_integer(pointer_level, arg, ch);
			case 'm': return format_integer(pointer_level, arg, ch);
			case 'x': return format_integer(pointer_level, arg, ch);
			case 'y': return format_integer(pointer_level, arg, ch);
			case 'n': return format_integer(pointer_level, arg, ch);
			case 'o': return format_integer(pointer_level, arg, ch);
			case 'f':
			case 'd':
			case 'e':
			case 'g':
			case 'z':
			default:
				break;		
			}
		}
	}
	
	return format_pointer(pointer_level, arg, 'x');
}

//------------------------------------------------------------------------------
// Name: resolve_function_parameters
// Desc:
//------------------------------------------------------------------------------
void resolve_function_parameters(const State &state, const QString &symname, int offset, QStringList &ret) {

	/*
	 * The calling convention of the AMD64 application binary interface is
	 * followed on Linux and other non-Microsoft operating systems.
	 * The registers RDI, RSI, RDX, RCX, R8 and R9 are used for integer and
	 * pointer arguments while XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and
	 * XMM7 are used for floating point arguments. As in the Microsoft x64
	 * calling convention, additional arguments are pushed onto the stack and
	 * the return value is stored in RAX.
	 */
	static const char *const parameter_registers[6] = {
		"rdi",
		"rsi",
		"rdx",
		"rcx",
		"r8",
		"r9"
	};

	// we will always be removing the last 2 chars '+0' from the string as well
	// as chopping the region prefix we like to prepend to symbols
	QString func_name;
	const int colon_index = symname.indexOf("::");

	if(colon_index != -1) {
		func_name = symname.left(symname.length() - 2).mid(colon_index + 2);
	}

	// safe not to check for -1, it means 'rest of string' for the mid function
	func_name = func_name.mid(0, func_name.indexOf("@"));

	if(const edb::Prototype *const info = edb::v1::get_function_info(func_name)) {

		QStringList arguments;
		int i = 0;
		Q_FOREACH(edb::Argument argument, info->arguments) {

			edb::reg_t arg;

			if(i > 5) {
				edb::v1::debugger_core->read_bytes(state.stack_pointer() + (i - 5) * sizeof(edb::reg_t) + offset, &arg, sizeof(arg));
			} else {
				arg = state[parameter_registers[i]].value<edb::reg_t>();
			}

			arguments << format_argument(argument.type, arg);
			++i;
		}
		ret << QString("%1(%2)").arg(func_name, arguments.join(", "));
	}
}

//------------------------------------------------------------------------------
// Name: is_jcc_taken
// Desc:
//------------------------------------------------------------------------------
bool is_jcc_taken(const State &state, quint8 instruction_byte) {

	const edb::reg_t efl = state.flags();
	const bool cf = (efl & 0x0001) != 0;
	const bool pf = (efl & 0x0004) != 0;
	const bool zf = (efl & 0x0040) != 0;
	const bool sf = (efl & 0x0080) != 0;
	const bool of = (efl & 0x0800) != 0;

	bool taken = false;

	switch(instruction_byte & 0x0e) {
	case 0x00:
		taken = of;
		break;
	case 0x02:
		taken = cf;
		break;
	case 0x04:
		taken = zf;
		break;
	case 0x06:
		taken = cf || zf;
		break;
	case 0x08:
		taken = sf;
		break;
	case 0x0a:
		taken = pf;
		break;
	case 0x0c:
		taken = sf != of;
		break;
	case 0x0e:
		taken = zf || sf != of;
		break;
	}

	if(instruction_byte & 0x01) {
		taken = !taken;
	}

	return taken;
}

//------------------------------------------------------------------------------
// Name: analyze_cmov
// Desc:
//------------------------------------------------------------------------------
void analyze_cmov(const State &state, const edb::Instruction &inst, QStringList &ret) {

	const quint8 *const buf = inst.bytes();

	const bool taken = is_jcc_taken(state, buf[1 + inst.prefix_size()]);

	if(taken) {
		ret << ArchProcessor::tr("move performed");
	} else {
		ret << ArchProcessor::tr("move NOT performed");
	}
}

//------------------------------------------------------------------------------
// Name: analyze_jump
// Desc:
//------------------------------------------------------------------------------
void analyze_jump(const State &state, const edb::Instruction &inst, QStringList &ret) {

	bool taken = false;

	const quint8 *const buf = inst.bytes();

	if(is_conditional_jump(inst)) {

		if(inst.size() - inst.prefix_size() == 2) {
			taken = is_jcc_taken(state, buf[0 + inst.prefix_size()]);
		} else {
			taken = is_jcc_taken(state, buf[1 + inst.prefix_size()]);
		}

	} else if(buf[0] == 0xe3) {
		if(inst.prefix() & edb::Instruction::PREFIX_ADDRESS) {
			taken = (state["rcx"].value<edb::reg_t>() & 0xffff) == 0;
		} else {
			taken = state["rcx"].value<edb::reg_t>() == 0;
		}
	}

	if(taken) {
		ret << ArchProcessor::tr("jump taken");
	} else {
		ret << ArchProcessor::tr("jump NOT taken");
	}
}

//------------------------------------------------------------------------------
// Name: analyze_return
// Desc:
//------------------------------------------------------------------------------
void analyze_return(const State &state, const edb::Instruction &inst, QStringList &ret) {
	Q_UNUSED(inst);

	edb::address_t return_address;
	edb::v1::debugger_core->read_bytes(state.stack_pointer(), &return_address, sizeof(return_address));

	const QString symname = edb::v1::find_function_symbol(return_address);
	if(!symname.isEmpty()) {
		ret << ArchProcessor::tr("return to %1 <%2>").arg(edb::v1::format_pointer(return_address)).arg(symname);
	} else {
		ret << ArchProcessor::tr("return to %1").arg(edb::v1::format_pointer(return_address));
	}
}

//------------------------------------------------------------------------------
// Name: analyze_call
// Desc:
//------------------------------------------------------------------------------
void analyze_call(const State &state, const edb::Instruction &inst, QStringList &ret) {

	const edb::Operand &operand = inst.operands()[0];

	if(operand.valid()) {

		const edb::address_t effective_address = get_effective_address(operand, state);
		const QString temp_operand             = QString::fromStdString(to_string(operand));
		QString temp;

		switch(operand.general_type()) {
		case edb::Operand::TYPE_REL:
		case edb::Operand::TYPE_REGISTER:
			do {
				int offset;
				const QString symname = edb::v1::find_function_symbol(effective_address, QString(), &offset);
				if(!symname.isEmpty()) {
					ret << QString("%1 = %2 <%3>").arg(temp_operand, edb::v1::format_pointer(effective_address), symname);

					if(offset == 0) {
						if(is_call(inst)) {
							resolve_function_parameters(state, symname, 0, ret);
						} else {
							resolve_function_parameters(state, symname, 4, ret);
						}
					}

				} else {
					ret << QString("%1 = %2").arg(temp_operand, edb::v1::format_pointer(effective_address));
				}
			} while(0);
			break;

		case edb::Operand::TYPE_EXPRESSION:
		default:
			do {
				edb::address_t target;
				const bool ok = edb::v1::debugger_core->read_bytes(effective_address, &target, sizeof(target));

				if(ok) {
					int offset;
					const QString symname = edb::v1::find_function_symbol(target, QString(), &offset);
					if(!symname.isEmpty()) {							
						ret << QString("%1 = [%2] = %3 <%4>").arg(temp_operand, edb::v1::format_pointer(effective_address), edb::v1::format_pointer(target), symname);

						if(offset == 0) {
							if(is_call(inst)) {
								resolve_function_parameters(state, symname, 0, ret);
							} else {
								resolve_function_parameters(state, symname, 4, ret);
							}
						}

					} else {
						ret << QString("%1 = [%2] = %3").arg(temp_operand, edb::v1::format_pointer(effective_address), edb::v1::format_pointer(target));
					}
				} else {
					// could not read from the address
					ret << QString("%1 = [%2] = ?").arg(temp_operand, edb::v1::format_pointer(effective_address));
				}
			} while(0);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// Name: analyze_operands
// Desc:
//------------------------------------------------------------------------------
void analyze_operands(const State &state, const edb::Instruction &inst, QStringList &ret) {

	Q_UNUSED(inst);

	for(int j = 0; j < edb::Instruction::MAX_OPERANDS; ++j) {

		const edb::Operand &operand = inst.operands()[j];

		if(operand.valid()) {

			const QString temp_operand = QString::fromStdString(to_string(operand));

			switch(operand.general_type()) {
			case edb::Operand::TYPE_REL:
			case edb::Operand::TYPE_REGISTER:
				do {
					const edb::address_t effective_address = get_effective_address(operand, state);
					ret << QString("%1 = %2").arg(temp_operand).arg(edb::v1::format_pointer(effective_address));
				} while(0);
				break;
			case edb::Operand::TYPE_EXPRESSION:
				do {
					const edb::address_t effective_address = get_effective_address(operand, state);
					edb::address_t target;

					const bool ok = edb::v1::debugger_core->read_bytes(effective_address, &target, sizeof(target));

					if(ok) {
						switch(operand.complete_type()) {
						case edb::Operand::TYPE_EXPRESSION8:
							ret << QString("%1 = [%2] = 0x%3").arg(temp_operand).arg(edb::v1::format_pointer(effective_address)).arg(target & 0xff, 2, 16, QChar('0'));
							break;
						case edb::Operand::TYPE_EXPRESSION16:
							ret << QString("%1 = [%2] = 0x%3").arg(temp_operand).arg(edb::v1::format_pointer(effective_address)).arg(target & 0xffff, 4, 16, QChar('0'));
							break;
						case edb::Operand::TYPE_EXPRESSION32:
							ret << QString("%1 = [%2] = 0x%3").arg(temp_operand).arg(edb::v1::format_pointer(effective_address)).arg(target & 0xffffffff, 8, 16, QChar('0'));
							break;
						case edb::Operand::TYPE_EXPRESSION64:
						default:
							ret << QString("%1 = [%2] = 0x%3").arg(temp_operand).arg(edb::v1::format_pointer(effective_address)).arg(target, 16, 16, QChar('0'));
							break;
						}
					} else {
						ret << QString("%1 = [%2] = ?").arg(temp_operand).arg(edb::v1::format_pointer(effective_address));
					}
				} while(0);
				break;
			default:
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
// Name: analyze_jump_targets
// Desc:
//------------------------------------------------------------------------------
void analyze_jump_targets(const edb::Instruction &inst, QStringList &ret)  {
	const edb::address_t address       = inst.rva();
	const edb::address_t start_address = address - 128;
	const edb::address_t end_address   = address + 127;

	quint8 buffer[edb::Instruction::MAX_SIZE];

	for(edb::address_t addr = start_address; addr < end_address; ++addr) {
		int sz = sizeof(buffer);
		if(edb::v1::get_instruction_bytes(addr, buffer, &sz)) {
			edb::Instruction inst(buffer, buffer + sz, addr, std::nothrow);
			if(is_jump(inst)) {
				const edb::Operand &operand = inst.operands()[0];

				if(operand.general_type() == edb::Operand::TYPE_REL) {
					const edb::address_t target = operand.relative_target();

					if(target == address) {
						ret << ArchProcessor::tr("possible jump from 0x%1").arg(edb::v1::format_pointer(addr));
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// Name: analyze_syscall
// Desc:
//------------------------------------------------------------------------------
void analyze_syscall(const State &state, const edb::Instruction &inst, QStringList &ret) {
	Q_UNUSED(inst);
	Q_UNUSED(ret);
	Q_UNUSED(state);

#ifdef Q_OS_LINUX	
	QString syscall_entry;
	QDomDocument syscall_xml;
	QFile file(":/debugger/xml/syscalls.xml");	
	if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {

		QXmlQuery query;
		QString res;
		query.setFocus(&file);
		query.setQuery(QString("syscalls[@version='1.0']/linux[@arch='x86-64']/syscall[index=%1]").arg(state["rax"].value<edb::reg_t>()));
		if (query.isValid()) {
			query.evaluateTo(&syscall_entry);
		}
		file.close();
	}
	
	if(!syscall_entry.isEmpty()) {
		syscall_xml.setContent("" + syscall_entry + "");
		QDomElement root = syscall_xml.documentElement();
		
		QStringList arguments;

		for (QDomElement argument = root.firstChildElement("argument"); !argument.isNull(); argument = argument.nextSiblingElement("argument")) {
			const QString argument_type     = argument.attribute("type");
			const QString argument_register = argument.attribute("register");
			arguments << format_argument(argument_type, state[argument_register].value<edb::reg_t>());
		}
		
		ret << ArchProcessor::tr("SYSCALL: %1(%2)").arg(root.attribute("name"), arguments.join(","));
	}
#endif
}

}

//------------------------------------------------------------------------------
// Name: ArchProcessor
// Desc:
//------------------------------------------------------------------------------
ArchProcessor::ArchProcessor() : split_flags_(0) {
	if(edb::v1::debugger_core) {
		has_mmx_ = edb::v1::debugger_core->has_extension(edb::string_hash<'M', 'M', 'X'>::value);
		has_xmm_ = edb::v1::debugger_core->has_extension(edb::string_hash<'X', 'M', 'M'>::value);
	} else {
		has_mmx_ = false;
		has_xmm_ = false;
	}
}

//------------------------------------------------------------------------------
// Name: setup_register_view
// Desc:
//------------------------------------------------------------------------------
void ArchProcessor::setup_register_view(RegisterListWidget *category_list) {

	if(edb::v1::debugger_core) {
		State state;

		Q_ASSERT(category_list);

		// setup the register view
		if(QTreeWidgetItem *const gpr = category_list->addCategory(tr("General Purpose"))) {
			register_view_items_[0x00] = create_register_item(gpr, "rax");
			register_view_items_[0x01] = create_register_item(gpr, "rbx");
			register_view_items_[0x02] = create_register_item(gpr, "rcx");
			register_view_items_[0x03] = create_register_item(gpr, "rdx");
			register_view_items_[0x04] = create_register_item(gpr, "rbp");
			register_view_items_[0x05] = create_register_item(gpr, "rsp");
			register_view_items_[0x06] = create_register_item(gpr, "rsi");
			register_view_items_[0x07] = create_register_item(gpr, "rdi");
			register_view_items_[0x08] = create_register_item(gpr, "r8");
			register_view_items_[0x09] = create_register_item(gpr, "r9");
			register_view_items_[0x0a] = create_register_item(gpr, "r10");
			register_view_items_[0x0b] = create_register_item(gpr, "r11");
			register_view_items_[0x0c] = create_register_item(gpr, "r12");
			register_view_items_[0x0d] = create_register_item(gpr, "r13");
			register_view_items_[0x0e] = create_register_item(gpr, "r14");
			register_view_items_[0x0f] = create_register_item(gpr, "r15");
			register_view_items_[0x10] = create_register_item(gpr, "rip");
			register_view_items_[0x11] = create_register_item(gpr, "rflags");

			// split rflags view
			split_flags_ = new QTreeWidgetItem(register_view_items_[0x11]);
			split_flags_->setText(0, state.flags_to_string(0));
		}

		if(QTreeWidgetItem *const segs = category_list->addCategory(tr("Segments"))) {
			register_view_items_[0x12] = create_register_item(segs, "cs");
			register_view_items_[0x13] = create_register_item(segs, "ds");
			register_view_items_[0x14] = create_register_item(segs, "es");
			register_view_items_[0x15] = create_register_item(segs, "fs");
			register_view_items_[0x16] = create_register_item(segs, "gs");
			register_view_items_[0x17] = create_register_item(segs, "ss");
		}

		if(QTreeWidgetItem *const fpu = category_list->addCategory(tr("FPU"))) {
			register_view_items_[0x18] = create_register_item(fpu, "st0");
			register_view_items_[0x19] = create_register_item(fpu, "st1");
			register_view_items_[0x1a] = create_register_item(fpu, "st2");
			register_view_items_[0x1b] = create_register_item(fpu, "st3");
			register_view_items_[0x1c] = create_register_item(fpu, "st4");
			register_view_items_[0x1d] = create_register_item(fpu, "st5");
			register_view_items_[0x1e] = create_register_item(fpu, "st6");
			register_view_items_[0x1f] = create_register_item(fpu, "st7");
		}
		
		if(QTreeWidgetItem *const dbg = category_list->addCategory(tr("Debug"))) {
			register_view_items_[0x20] = create_register_item(dbg, "dr0");
			register_view_items_[0x21] = create_register_item(dbg, "dr1");
			register_view_items_[0x22] = create_register_item(dbg, "dr2");
			register_view_items_[0x23] = create_register_item(dbg, "dr3");
			register_view_items_[0x24] = create_register_item(dbg, "dr4");
			register_view_items_[0x25] = create_register_item(dbg, "dr5");
			register_view_items_[0x26] = create_register_item(dbg, "dr6");
			register_view_items_[0x27] = create_register_item(dbg, "dr7");
		}

		if(has_mmx_) {
			if(QTreeWidgetItem *const mmx = category_list->addCategory(tr("MMX"))) {
				register_view_items_[0x28] = create_register_item(mmx, "mm0");
				register_view_items_[0x29] = create_register_item(mmx, "mm1");
				register_view_items_[0x2a] = create_register_item(mmx, "mm2");
				register_view_items_[0x2b] = create_register_item(mmx, "mm3");
				register_view_items_[0x2c] = create_register_item(mmx, "mm4");
				register_view_items_[0x2d] = create_register_item(mmx, "mm5");
				register_view_items_[0x2e] = create_register_item(mmx, "mm6");
				register_view_items_[0x2f] = create_register_item(mmx, "mm7");
			}
		}

		if(has_xmm_) {
			if(QTreeWidgetItem *const xmm = category_list->addCategory(tr("XMM"))) {
				register_view_items_[0x30] = create_register_item(xmm, "xmm0");
				register_view_items_[0x31] = create_register_item(xmm, "xmm1");
				register_view_items_[0x32] = create_register_item(xmm, "xmm2");
				register_view_items_[0x33] = create_register_item(xmm, "xmm3");
				register_view_items_[0x34] = create_register_item(xmm, "xmm4");
				register_view_items_[0x35] = create_register_item(xmm, "xmm5");
				register_view_items_[0x36] = create_register_item(xmm, "xmm6");
				register_view_items_[0x37] = create_register_item(xmm, "xmm7");
				register_view_items_[0x38] = create_register_item(xmm, "xmm8");
				register_view_items_[0x39] = create_register_item(xmm, "xmm9");
				register_view_items_[0x3a] = create_register_item(xmm, "xmm10");
				register_view_items_[0x3b] = create_register_item(xmm, "xmm11");
				register_view_items_[0x3c] = create_register_item(xmm, "xmm12");
				register_view_items_[0x3d] = create_register_item(xmm, "xmm13");
				register_view_items_[0x3e] = create_register_item(xmm, "xmm14");
				register_view_items_[0x3f] = create_register_item(xmm, "xmm15");
			}
		}

		update_register_view(QString(), State());
	}
}

//------------------------------------------------------------------------------
// Name: value_from_item
// Desc:
//------------------------------------------------------------------------------
Register ArchProcessor::value_from_item(const QTreeWidgetItem &item) {
	const QString &name = item.data(0, Qt::UserRole).toString();
	State state;
	edb::v1::debugger_core->get_state(&state);
	return state[name];
}

//------------------------------------------------------------------------------
// Name: update_register
// Desc:
//------------------------------------------------------------------------------
void ArchProcessor::update_register(QTreeWidgetItem *item, const QString &name, const Register &reg) const {

	Q_ASSERT(item);

	QString reg_string;
	int string_length;
	const edb::reg_t value = reg.value<edb::reg_t>();

	if(edb::v1::get_ascii_string_at_address(value, reg_string, edb::v1::config().min_string_length, 256, string_length)) {
		item->setText(0, QString("%1: %2 ASCII \"%3\"").arg(name, edb::v1::format_pointer(value), reg_string));
	} else if(edb::v1::get_utf16_string_at_address(value, reg_string, edb::v1::config().min_string_length, 256, string_length)) {
		item->setText(0, QString("%1: %2 UTF16 \"%3\"").arg(name, edb::v1::format_pointer(value), reg_string));
	} else {
		item->setText(0, QString("%1: %2").arg(name, edb::v1::format_pointer(value)));
	}
}

//------------------------------------------------------------------------------
// Name: reset
// Desc:
//------------------------------------------------------------------------------
void ArchProcessor::reset() {

	if(edb::v1::debugger_core) {
		last_state_.clear();
		update_register_view(QString(), State());
	}
}

//------------------------------------------------------------------------------
// Name: update_register_view
// Desc:
//------------------------------------------------------------------------------
void ArchProcessor::update_register_view(const QString &default_region_name, const State &state) {
	const QPalette palette = QApplication::palette();

	update_register(register_view_items_[0], "RAX", state["rax"]);
	update_register(register_view_items_[1], "RBX", state["rbx"]);
	update_register(register_view_items_[2], "RCX", state["rcx"]);
	update_register(register_view_items_[3], "RDX", state["rdx"]);
	update_register(register_view_items_[4], "RBP", state["rbp"]);
	update_register(register_view_items_[5], "RSP", state["rsp"]);
	update_register(register_view_items_[6], "RSI", state["rsi"]);
	update_register(register_view_items_[7], "RDI", state["rdi"]);
	update_register(register_view_items_[8], "R8 ", state["r8"]);
	update_register(register_view_items_[9], "R9 ", state["r9"]);
	update_register(register_view_items_[10], "R10", state["r10"]);
	update_register(register_view_items_[11], "R11", state["r11"]);
	update_register(register_view_items_[12], "R12", state["r12"]);
	update_register(register_view_items_[13], "R13", state["r13"]);
	update_register(register_view_items_[14], "R14", state["r14"]);
	update_register(register_view_items_[15], "R15", state["r15"]);

	const QString symname = edb::v1::find_function_symbol(state.instruction_pointer(), default_region_name);

	if(!symname.isEmpty()) {
		register_view_items_[16]->setText(0, QString("RIP: %1 <%2>").arg(edb::v1::format_pointer(state.instruction_pointer())).arg(symname));
	} else {
		register_view_items_[16]->setText(0, QString("RIP: %1").arg(edb::v1::format_pointer(state.instruction_pointer())));
	}
	register_view_items_[17]->setText(0, QString("RFLAGS: %1").arg(edb::v1::format_pointer(state.flags())));

	register_view_items_[18]->setText(0, QString("CS: %1")     .arg(state["cs"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')));
	register_view_items_[19]->setText(0, QString("DS: %1")     .arg(state["ds"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')));
	register_view_items_[20]->setText(0, QString("ES: %1")     .arg(state["es"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')));
	register_view_items_[21]->setText(0, QString("FS: %1 (%2)").arg(state["fs"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')).arg(edb::v1::format_pointer(state["fs_base"].value<edb::reg_t>())));
	register_view_items_[22]->setText(0, QString("GS: %1 (%2)").arg(state["gs"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')).arg(edb::v1::format_pointer(state["gs_base"].value<edb::reg_t>())));
	register_view_items_[23]->setText(0, QString("SS: %1")     .arg(state["ss"].value<edb::reg_t>() & 0xffff, 4, 16, QChar('0')));

	for(int i = 0; i < 8; ++i) {
		const long double current = state.fpu_register(i);
		const long double prev    = last_state_.fpu_register(i);
		register_view_items_[24 + i]->setText(0, QString("ST%1: %2").arg(i).arg(current, 0, 'g', 16));
		register_view_items_[24 + i]->setForeground(0, QBrush((current != prev && !(boost::math::isnan(prev) && boost::math::isnan(current))) ? Qt::red : palette.text()));
	}

	for(int i = 0; i < 8; ++i) {
		register_view_items_[32 + i]->setText(0, QString("DR%1: %2").arg(i).arg(state.debug_register(i), 0, 16));
		register_view_items_[32 + i]->setForeground(0, QBrush((state.debug_register(i) != last_state_.debug_register(i)) ? Qt::red : palette.text()));
	}

	if(has_mmx_) {
		for(int i = 0; i < 8; ++i) {
			const quint64 current = state.mmx_register(i);
			const quint64 prev    = last_state_.mmx_register(i);
			register_view_items_[40 + i]->setText(0, QString("MM%1: %2").arg(i).arg(current, sizeof(quint64)*2, 16, QChar('0')));
			register_view_items_[40 + i]->setForeground(0, QBrush((current != prev) ? Qt::red : palette.text()));
		}
	}

	if(has_xmm_) {
		for(int i = 0; i < 16; ++i) {
			const QByteArray current = state.xmm_register(i);
			const QByteArray prev    = last_state_.xmm_register(i);
			Q_ASSERT(current.size() == 16 || current.size() == 0);
			register_view_items_[48 + i]->setText(0, QString("XMM%1: %2").arg(i, -2).arg(current.toHex().constData()));
			register_view_items_[48 + i]->setForeground(0, QBrush((current != prev) ? Qt::red : palette.text()));
		}
	}

	const bool flags_changed = state.flags() != last_state_.flags();
	if(flags_changed) {
		split_flags_->setText(0, state.flags_to_string());
	}

	// highlight any changed registers
	register_view_items_[0x00]->setForeground(0, QBrush((state["rax"] != last_state_["rax"]) ? Qt::red : palette.text()));
	register_view_items_[0x01]->setForeground(0, QBrush((state["rbx"] != last_state_["rbx"]) ? Qt::red : palette.text()));
	register_view_items_[0x02]->setForeground(0, QBrush((state["rcx"] != last_state_["rcx"]) ? Qt::red : palette.text()));
	register_view_items_[0x03]->setForeground(0, QBrush((state["rdx"] != last_state_["rdx"]) ? Qt::red : palette.text()));
	register_view_items_[0x04]->setForeground(0, QBrush((state["rbp"] != last_state_["rbp"]) ? Qt::red : palette.text()));
	register_view_items_[0x05]->setForeground(0, QBrush((state["rsp"] != last_state_["rsp"]) ? Qt::red : palette.text()));
	register_view_items_[0x06]->setForeground(0, QBrush((state["rsi"] != last_state_["rsi"]) ? Qt::red : palette.text()));
	register_view_items_[0x07]->setForeground(0, QBrush((state["rdi"] != last_state_["rdi"]) ? Qt::red : palette.text()));
	register_view_items_[0x08]->setForeground(0, QBrush((state["r8"] != last_state_["r8"]) ? Qt::red : palette.text()));
	register_view_items_[0x09]->setForeground(0, QBrush((state["r9"] != last_state_["r9"]) ? Qt::red : palette.text()));
	register_view_items_[0x0a]->setForeground(0, QBrush((state["r10"] != last_state_["r10"]) ? Qt::red : palette.text()));
	register_view_items_[0x0b]->setForeground(0, QBrush((state["r11"] != last_state_["r11"]) ? Qt::red : palette.text()));
	register_view_items_[0x0c]->setForeground(0, QBrush((state["r12"] != last_state_["r12"]) ? Qt::red : palette.text()));
	register_view_items_[0x0d]->setForeground(0, QBrush((state["r13"] != last_state_["r13"]) ? Qt::red : palette.text()));
	register_view_items_[0x0e]->setForeground(0, QBrush((state["r14"] != last_state_["r14"]) ? Qt::red : palette.text()));
	register_view_items_[0x0f]->setForeground(0, QBrush((state["r15"] != last_state_["r15"]) ? Qt::red : palette.text()));
	register_view_items_[0x10]->setForeground(0, QBrush((state.instruction_pointer() != last_state_.instruction_pointer()) ? Qt::red : palette.text()));
	register_view_items_[0x11]->setForeground(0, QBrush(flags_changed ? Qt::red : palette.text()));

	last_state_ = state;
}

//------------------------------------------------------------------------------
// Name: update_instruction_info
// Desc:
//------------------------------------------------------------------------------
QStringList ArchProcessor::update_instruction_info(edb::address_t address) {

	QStringList ret;

	Q_ASSERT(edb::v1::debugger_core);

	quint8 buffer[edb::Instruction::MAX_SIZE];

	const bool ok = edb::v1::debugger_core->read_bytes(address, buffer, sizeof(buffer));
	if(ok) {
		edb::Instruction inst(buffer, buffer + sizeof(buffer), address, std::nothrow);
		if(inst) {

			State state;
			edb::v1::debugger_core->get_state(&state);

			// figure out the instruction type and display some information about it
			switch(inst.type()) {
			case edb::Instruction::OP_CMOVCC:
				analyze_cmov(state, inst, ret);
				break;
			case edb::Instruction::OP_RET:
				analyze_return(state, inst, ret);
				break;
			case edb::Instruction::OP_JCC:
				analyze_jump(state, inst, ret);
				// FALL THROUGH!
			case edb::Instruction::OP_JMP:
			case edb::Instruction::OP_CALL:
				analyze_call(state, inst, ret);
				break;
			#ifdef Q_OS_LINUX
			case edb::Instruction::OP_INT:
				if(inst.operands()[0].complete_type() == edb::Operand::TYPE_IMMEDIATE8 && (inst.operands()[0].immediate() & 0xff) == 0x80) {
					analyze_syscall(state, inst, ret);
				} else {
					analyze_operands(state, inst, ret);
				}
				break;
			#endif
			case edb::Instruction::OP_SYSCALL:
				analyze_syscall(state, inst, ret);
				break;
			default:
				analyze_operands(state, inst, ret);
				break;
			}

			analyze_jump_targets(inst, ret);

		}
	}

	// eliminate duplicates
	ret = QStringList::fromSet(ret.toSet());

	return ret;
}

//------------------------------------------------------------------------------
// Name: can_step_over
// Desc:
//------------------------------------------------------------------------------
bool ArchProcessor::can_step_over(const edb::Instruction &inst) const {
	return inst && (is_call(inst) || (inst.prefix() & (edb::Instruction::PREFIX_REPNE | edb::Instruction::PREFIX_REP)));
}

//------------------------------------------------------------------------------
// Name: is_filling
// Desc:
//------------------------------------------------------------------------------
bool ArchProcessor::is_filling(const edb::Instruction &inst) const {
	bool ret = false;

	// fetch the operands
	if(inst) {
		const edb::Operand operands[edb::Instruction::MAX_OPERANDS] = {
			inst.operands()[0],
			inst.operands()[1],
			inst.operands()[2]
		};

		switch(inst.type()) {
		case edb::Instruction::OP_NOP:
		case edb::Instruction::OP_INT3:
			ret = true;
			break;

		case edb::Instruction::OP_LEA:
			if(operands[0].valid() && operands[1].valid()) {
				if(operands[0].general_type() == edb::Operand::TYPE_REGISTER && operands[1].general_type() == edb::Operand::TYPE_EXPRESSION) {

					edb::Operand::Register reg1;
					edb::Operand::Register reg2;

					reg1 = operands[0].reg();

					if(operands[1].expression().scale == 1) {
						if(operands[1].expression().u_disp32 == 0) {

							if(operands[1].expression().base == edb::Operand::REG_NULL) {
								reg2 = operands[1].expression().index;
								ret = reg1 == reg2;
							} else if(operands[1].expression().index == edb::Operand::REG_NULL) {
								reg2 = operands[1].expression().base;
								ret = reg1 == reg2;
							}
						}
					}
				}
			}
			break;

		case edb::Instruction::OP_MOV:
			if(operands[0].valid() && operands[1].valid()) {
				if(operands[0].general_type() == edb::Operand::TYPE_REGISTER && operands[1].general_type() == edb::Operand::TYPE_REGISTER) {
					ret = operands[0].reg() == operands[1].reg();
				}
			}
			break;

		default:
			break;
		}

		if(!ret) {
			if(edb::v1::config().zeros_are_filling) {
				ret = (QByteArray::fromRawData(reinterpret_cast<const char *>(inst.bytes()), inst.size()) == QByteArray::fromRawData("\x00\x00", 2));
			}
		}
	} else {
		ret = (inst.size() == 1 && inst.bytes()[0] == 0x00);
	}

	return ret;
}
