/// gigStringExecute(string)
// executes the given string
// requires gigInit to have been called.
// returns false on success, or a string if there is an error.
// call gigReturnValue() to get the return value after a success.

var gi = external_call(global.dll_gigGenerate, argument0);

if (external_call(global.dll_gigError, gi))
{
    return external_call(global.dll_gigWhatError, gi);
}

var stack, locals;
var vInstanceName, vGlobalName, vBuiltInInstanceName;
var pc = 0, sc = 0, wc = 0;
var _self = self;
var _other = other;

// size of with iterator
var withcount;
// index of with iterator
var withidx;
// with iterator list
var withv;
// with tombstone
var withtomb;
withtomb[0] = false;

// do not extend this list.
vBuiltInInstanceName[0] = "id";
vBuiltInInstanceName[1] = "object_index";

// read in instance and global names
var vInstanceNameCount = external_call(global.dll_gigVariableInstanceCount, gi);
var vGlobalNameCount = external_call(global.dll_gigVariableGlobalCount, gi);
for (var i = 0; i < vInstanceNameCount; i++)
{
    vInstanceName[i] = external_call(global.dll_gigVariableInstanceName, gi, i);
}
for (var i = 0; i < vGlobalNameCount; i++)
{
    vGlobalName[i] = external_call(global.dll_gigVariableGlobalName, gi, i);
}

// read in instructions
// these map from instruction address to data
var instructionCount = external_call(global.dll_gigInstructionCount, gi);
var maxAddress = external_call(global.dll_gigInstructionAddress, gi, instructionCount - 1);
var addressOpcode, addressAligned, addressImmediate;

// initialize arrays
addressOpcode[maxAddress] = 0;
addressAligned[maxAddress] = 0;
addressImmediate[maxAddress] = 0;

// fill arrays
for (var i = 0; i < instructionCount; i++)
{
    var address = external_call(global.dll_gigInstructionAddress, gi, i);
    addressAligned[address] = true;
    addressOpcode[address] = external_call(global.dll_gigInstructionOpcode, gi, i);
    addressImmediate[address] = external_call(global.dll_gigInstructionImmediate, gi, i);
}

// condition flag
var fc = 0;h

// execute
while (true)
{
    if (pc > maxAddress)
    {
        return "program counter exceeded bytecode section range during execution.";
    }

    // read in opcode
    var opcode = addressOpcode[pc];
    var immediate = addressImmediate[pc];

    // execute opcode
    switch(opcode)
    {
    case "ldi_false":
        stack[sc++] = false;
        break;
    case "ldi_true":
        stack[sc++] = true;
        break;
    case "ldi_undef":
        stack[sc++] = undefined
        break;
    case "ldi_f32":
    case "ldi_s32":
    case "ldi_u64":
        // TODO: load s32, u64 properly?
        stack[sc++] = real(immediate);
        break;
    case "ldi_string":
        stack[sc++] = immediate;
        break;
    case "inc":
        stack[sc - 1]++;
        break;
    case "dec":
        stack[sc - 1]--;
        break;
    case "add2":
        stack[sc - 2] = stack[sc - 2] + stack[sc - 1];
        sc--;
        break;
    case "sub2":
        stack[sc - 2] = stack[sc - 2] - stack[sc - 1];
        sc--;
        break;
    case "mult2":
        stack[sc - 2] = stack[sc - 2] * stack[sc - 1];
        sc--;
        break;
    case "fdiv2":
        stack[sc - 2] = stack[sc - 2] / stack[sc - 1];
        sc--;
        break;
    case "idiv2":
        stack[sc - 2] = stack[sc - 2] div stack[sc - 1];
        sc--;
        break;
    case "mod2":
        stack[sc - 2] = stack[sc - 2] mod stack[sc - 1];
        sc--;
        break;
    case "lsh2":
        stack[sc - 2] = stack[sc - 2] << stack[sc - 1];
        sc--;
        break;
    case "rsh2":
        stack[sc - 2] = stack[sc - 2] >> stack[sc - 1];
        sc--;
        break;
    case "lt":
        fc = stack[sc - 2] < stack[sc - 1];
        sc-=2;
        break;
    case "lte":
        fc = stack[sc - 2] <= stack[sc - 1];
        sc-=2;
        break;
    case "gt":
        fc = stack[sc - 2] > stack[sc - 1];
        sc-=2;
        break;
    case "gte":
        fc = stack[sc - 2] >= stack[sc - 1];
        sc-=2;
        break;
    case "eq":
        fc = stack[sc - 2] == stack[sc - 1];
        sc-=2;
        break;
    case "neq":
        fc = stack[sc - 2] != stack[sc - 1];
        sc-=2;
        break;
    case "bland":
        stack[sc - 2] = stack[sc - 2] && stack[sc - 1];
        sc--;
        break;
    case "blor":
        stack[sc - 2] = stack[sc - 2] || stack[sc - 1];
        sc--;
        break;
    case "blxor":
        stack[sc - 2] = stack[sc - 2] ^^ stack[sc - 1];
        sc--;
        break;
    case "band":
        stack[sc - 2] = stack[sc - 2] & stack[sc - 1];
        sc--;
        break;
    case "bor":
        stack[sc - 2] = stack[sc - 2] | stack[sc - 1];
        sc--;
        break;
    case "bxor":
        stack[sc - 2] = stack[sc - 2] ^ stack[sc - 1];
        sc--;
        break;
    case "bnot":
        stack[sc - 1] = ~(stack[sc - 1]);
        break;
    case "cond":
        if (stack[sc - 1])
        {
            fc = true;
        }
        else
        {
            fc = false;
        }
        sc--;
        break;
    case "ncond":
        if (stack[sc - 1])
        {
            fc = false;
        }
        else
        {
            fc = true;
        }
        sc--;
        break;
    case "pcond":
        stack[sc++] = fc;
        break;
    case "ste":
        locals[real(immediate)] = stack[--sc];
        break;
    case "lde":
        stack[sc++] = locals[real(immediate)];
        break;
    case "sts":
        gigVariableSet(vInstanceName[real(immediate)], stack[--sc], false, _self);
        break;
    case "lds":
        stack[sc++] = gigVariableGet(vInstanceName[real(immediate)], false, _self);
        break;
    case "sto":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            gigVariableSet(vInstanceName[real(immediate)], val, false, _id);
        }
        break;
    case "ldo":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            stack[sc++] = gigVariableGet(vInstanceName[real(immediate)], false, _id);
            break;
        }
        break;
    case "stg":
        gigVariableSet(vGlobalName[real(immediate)], stack[--sc], true);
        break;
    case "ldg":
        stack[sc++] = gigVariableSet(vGlobalName[real(immediate)], true);
        break;
    case "stt":
        gigVariableSet(vBuiltInInstanceName[real(immediate)], stack[--sc], false, _self);
        break;
    case "ldt":
        stack[sc++] = gigVariableGet(vBuiltInInstanceName[real(immediate)], false, _self);
        break;
    case "stp":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            gigVariableSet(vBuiltInInstanceName[real(immediate)], val, false, _id);
        }
        break;
    case "ldp":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            stack[sc++] = gigVariableGet(vBuiltInInstanceName[real(immediate)], false, _id);
            break;
        }
        break;
    case "stsa":
        {
            var val = stack[--sc];
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            gigVariableSet(vInstanceName[real(immediate)], val, false, _self, ix1, ix2);
        }
        break;
    case "ldsa":
        {
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            stack[sc++] = gigVariableGet(vInstanceName[real(immediate)], false, _self, ix1, ix2);
        }
        break;
    case "stoa":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            gigVariableSet(vInstanceName[real(immediate)], val, false, _id, ix1, ix2);
        }
        break;
    case "ldoa":
        {
            var val = stack[--sc];
            var _id = stack[--sc];
            if (_id == self)
            {
                _id = _self;
            }
            if (_id == other)
            {
                _id = _other;
            }
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            stack[sc++] = gigVariableGet(vInstanceName[real(immediate)], false, _id, ix1, ix2);
            break;
        }
        break;
    case "stga":
        {
            var val = stack[--sc];
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            gigVariableSet(vGlobalName[real(immediate)], val, true, ix1, ix2);
            break;
        }
    case "ldga":
        {
            var ix2 = stack[--sc];
            var ix1 = stack[--sc];
            stack[sc++] = gigVariableSet(vGlobalName[real(immediate)], true, ix1, ix2);
            break;
        }
    case "pop":
        sc--;
        break;
    case "dup":
        sc++;
        stack[sc] = stack[sc - 1];
    case "dup2":
        sc += 2;
        stack[sc - 2] = stack[sc - 4];
        stack[sc - 1] = stack[sc - 3];
        break;
    case "dup3":
        sc += 3;
        stack[sc - 3] = stack[sc - 6];
        stack[sc - 2] = stack[sc - 5];
        stack[sc - 1] = stack[sc - 4];
        break;
    case "dupn":
        {
            var n = floor(real(immediate));
            sc += n;
            for (var i = 1; i <= n; i++)
            {
                stack[sc - i] = stack[sc - i - n];
            }
        }
        break;
    case "nat":
        {
            var fn = "";
            var argc = 0;
            for (var i = 1; i <= string_length(immediate); i++)
            {
                var c = string_char_at(immediate, i);
                if (c == ':')
                {
                    var argcs = string_copy(immediate, i + 1, string_length(immediate) - i);
                    argc = floor(real(argcs));
                    break;
                }
                fn += c;
            }
            var argv;
            argv[0] = 0;
            for (var i = 0; i < argc; i++)
            {
                argv[i] = stack[sc - argc + i];
            }
            global.dll_gigExecutionError = false;
            gigFunction(fn, argc, argv);
            if (global.dll_gigExecutionError == 1)
            {
                return "The function '" + fn  + "'could not be executed, as it is not a user-defined script nor is it listed in the script gigFunction. Please consider adding it to the script gigFunction if it is a built-in script.";
            }
            if (global.dll_gigExecutionError == 2)
            {
                return "The function '" + fn  + "' was invoked with " + string(argc) + " arguments, which is too many for the interpreter to handle.";
            }
        }
        break;
    case "wti":
        {
            // find an iterator index
            var found = false;
            var wi = 0;
            for (var i = 0; i < wc; i++)
            {
                if (!withtomb[i])
                {
                    found = true;
                    wi = i;
                    break;
                }
            }
            if (!found)
            {
                wi = wc++;
            }

            // construct iterator
            var wcond = stack[--sc];
            withidx[wi] = 0;
            withcount[wi] = 0;
            withtomb[wi] = false;
            with (wcond)
            {
                withv[wi, withcount[wi]++] = id;
            }

            stack[sc++] = _other;
            stack[sc++] = _self;

            // push current id onto stack
            stack[sc++] = wi;
        }
        break;
    case "wty":
        {
            // retrive iterator index
            var wi = stack[sc - 1];
            var valid = true;

            if (wi >= wc)
            {
                valid = false;
            }
            else if (withtomb[wi])
            {
                valid = false;
            }
            if (!valid)
            {
                return "yield from invalid with-iterator.";
            }
            if (withidx[wi] >= withcount[wi])
            {
                // iterator elapsed. Free iterator, restore self.
                withtomb[wi] = true;

                // pop iterator
                --sc;

                // pop self and other
                _self = stack[--sc];
                _other = stack[--sc];

                fc = true;
            }
            else
            {
                // set self
                _self = withv[wi, withidx[wi]++];

                fc = false;
            }
        }
        break;
    case "wtd":
        {
            var wi = stack[--sc];
            _self = stack[--sc];
            _other = stack[--sc];
            withtomb[wi] = true;
        }
        break;
    case "jmp":
        // subtract 1 to account for pc advancement
        pc = floor(real(immediate)) - 1;
        break;
    case "bcond":
        if (fc)
        {
            // subtract 1 to account for pc advancement
            pc = floor(real(immediate)) - 1;
        }
        break;
    case "ret":
        if (sc != 1)
            return "Bytecode execution finished with " + string(sc) + " values on stack."
        global.dll_gigReturnValue = stack[0];
        return false;
    case "eof":
        return "bytecode eof reached -- unexpected.";
    case "nop":
        break;
    case "incl":
    case "decl":
    case "stl":
    case "ldl":
    case "stla":
    case "ldla":
    case "stpa":
    case "ldpa":
    case "pshl":
    case "popl":
    case "pshe":
    case "pope":
    case "call":
        return "opcode " + opcode + " was requested not to be compiled, but was encountered anyway."
    case "sfx":
    case "ufx":
        return "write-no-copy array accessor not supported (though it could be in the future.)";
    default:
        return "opcode not implemented: " + opcode + " at address " + string(pc);
    }

    do
    {
        pc += 1;
    } until (addressAligned[pc]);
}

return false;
