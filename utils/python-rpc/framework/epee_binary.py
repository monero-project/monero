import io

PORTABLE_STORAGE_SIGNATURE = bytes.fromhex('0111010101010201') # bender's nightmare 
PORTABLE_STORAGE_FORMAT_VER = 1

PORTABLE_RAW_SIZE_MARK_MASK   = 0x03 
PORTABLE_RAW_SIZE_MARK_BYTE   = 0
PORTABLE_RAW_SIZE_MARK_WORD   = 1
PORTABLE_RAW_SIZE_MARK_DWORD  = 2
PORTABLE_RAW_SIZE_MARK_INT64  = 3

SERIALIZE_TYPE_INT64                = 1
SERIALIZE_TYPE_INT32                = 2
SERIALIZE_TYPE_INT16                = 3
SERIALIZE_TYPE_INT8                 = 4
SERIALIZE_TYPE_UINT64               = 5
SERIALIZE_TYPE_UINT32               = 6
SERIALIZE_TYPE_UINT16               = 7
SERIALIZE_TYPE_UINT8                = 8
SERIALIZE_TYPE_DOUBLE               = 9
SERIALIZE_TYPE_STRING               = 10
SERIALIZE_TYPE_BOOL                 = 11
SERIALIZE_TYPE_OBJECT               = 12
#SERIALIZE_TYPE_ARRAY                = 13

SERIALIZE_FLAG_ARRAY              = 0x80

class Serializer:
    def __init__(self, outf = None):
        self.outf = outf if outf is not None else io.BytesIO()

    def serialize_stream(self, obj):
        self.outf.write(PORTABLE_STORAGE_SIGNATURE)
        self.outf.write(bytes([PORTABLE_STORAGE_FORMAT_VER]))
        self.__serialize_section(obj, include_type=False)

    def serialize(self, obj):
        assert(isinstance(self.outf, io.BytesIO))
        self.serialize_stream(obj)
        return self.outf.getvalue()

    @classmethod
    def __is_maybe_dict_like(cls, x):
        return hasattr(x, 'keys')

    @classmethod
    def __is_maybe_array_like(cls, x):
        return hasattr(x, '__iter__') \
            and not hasattr(x, 'encode') \
            and not hasattr(x, 'decode') \
            and not cls.__is_maybe_dict_like(x)

    @classmethod
    def __get_int_serialize_type(cls, signed, byte_size):
        if byte_size == 8:
            int_type = 1
        elif byte_size == 4:
            int_type = 2
        elif byte_size == 2:
            int_type = 3
        elif byte_size == 1:
            int_type = 4
        else:
            raise ValueError("Unrecognized serialized int byte size: " + str(byte_size))
        if not signed:
            int_type += 4
        return int_type

    def __serialize_fixed_int(self, x, signed, byte_size, include_type):
        int_type = self.__get_int_serialize_type(signed, byte_size)

        ones_mask = 2**(byte_size*8) - 1
        if signed:
            max_val = ones_mask // 2
            min_val = -(max_val+1)
        else:
            max_val = ones_mask
            min_val = 0

        if not (min_val <= x <= max_val):
            raise ValueError("Cannot serialize {}integer {} in {} bytes".format('un' if not signed else '', x, byte_size))

        if signed and x < 0:
            twos_comp = ones_mask+x+1
        else:
            twos_comp = x

        if include_type:
            self.outf.write(bytes([int_type]))

        int_bytes = twos_comp.to_bytes(byte_size, byteorder='little')
        self.outf.write(int_bytes)

    @classmethod
    def __get_auto_int_properties(cls, x):
        signed = x < 0
        if signed:
            x_lim = -2 * x
        else:
            x_lim = x

        byte_size = 1
        while byte_size <= 4:
            if x_lim <= 2**(byte_size*8)-1:
                break
            byte_size *= 2
        
        return signed, byte_size

    def __serialize_fixed_int_auto(self, x):
        signed, byte_size = self.__get_auto_int_properties(x)
        self.__serialize_fixed_int(x, signed, byte_size, include_type=True)

    def __serialize_varint(self, x):
        if x < 0:
            raise ValueError("Negative values cannot be serialized as a varint")
        for size_marker in range(4):
            byte_size = 1 << size_marker
            max_val = 2**(byte_size*8-2)-1
            if x > max_val:
                continue
            x_serial = (x << 2) + size_marker
            self.__serialize_fixed_int(x_serial, False, byte_size, False)
            return
        raise ValueError("Variant too large to be serialized")

    def __serialize_bool(self, x, include_type):
        if include_type:
            self.outf.write(bytes([SERIALIZE_TYPE_BOOL]))
        self.outf.write(bytes([1 if x else 0]))

    def __serialize_float(self, x, include_type):
        raise ValueError("Floating point numbers not supported yet")

    def __serialize_string(self, x, include_type):
        if include_type:
            self.outf.write(bytes([SERIALIZE_TYPE_STRING]))
        self.__serialize_varint(len(x))
        if isinstance(x, str):
            x = x.encode()
        self.outf.write(x)

    def __dispatch_serialize_scalar(self, x, include_type, signed_override = None, byte_size_override = None):
        if isinstance(x, bool):
            self.__serialize_bool(x, include_type)
        elif isinstance(x, int):
            if signed_override is None:
                self.__serialize_fixed_int_auto(x)
            else:
                self.__serialize_fixed_int(x, signed_override, byte_size_override, include_type)
        elif isinstance(x, float):
            self.__serialize_float(x, include_type)
        elif isinstance(x, bytes) or isinstance(x, str):
            self.__serialize_string(x, include_type)
        elif self.__is_maybe_dict_like(x):
            self.__serialize_section(x, include_type=True)
        else:
            raise ValueError("Cannot decide how to dispatch serialization for type {}".format(type(x)))

    def __serialize_section_key(self, x):
        if isinstance(x, str):
            x = x.encode()
        if len(x) > 255:
            raise ValueError("Object/section key name cannot be longer than 255 ASCII characters")
        self.__serialize_fixed_int(len(x), False, 1, False)
        self.outf.write(x)

    def __serialize_section_value(self, x):
        if self.__is_maybe_array_like(x):
            self.__serialize_array(x)
        else:
            self.__dispatch_serialize_scalar(x, include_type = True)

    def __serialize_section(self, x, include_type):
        if include_type:
            self.outf.write(bytes([SERIALIZE_TYPE_OBJECT]))
        self.__serialize_varint(len(x))
        for key, value in x.items():
            self.__serialize_section_key(key)
            self.__serialize_section_value(value)

    def __serialize_array(self, x):
        # Cannot determine the serialization type of a dynamically typed empty array
        # Thankfully, monero deserialization code allows coercing empty arrays to any type
        if len(x) == 0:
            EMPTY_U8_ARRAY_FLAG = SERIALIZE_FLAG_ARRAY | SERIALIZE_TYPE_UINT8
            self.__serialize_fixed_int(EMPTY_U8_ARRAY_FLAG, signed = False, byte_size = 1, include_type = False) #type
            self.__serialize_varint(0) # length

        signed = False
        byte_size = 1
        base_type = None

        # TODO: This doesn't assert that arrays are homogenous

        if isinstance[x[0], bool]:
            base_type = SERIALIZE_TYPE_BOOL
        elif isinstance(x[0], int):
            # Get automatic int type for entire array
            for elem in x:
                s, b = self.__get_auto_int_properties(elem)
                if s:
                    signed = True
                byte_size = max(byte_size, b)
            base_type = self.__get_int_serialize_type(signed, byte_size)
        elif isinstance(x[0], float):
            base_type = SERIALIZE_TYPE_DOUBLE
        elif isinstance(x[0], bytes) or isinstance(x[0], str):
            base_type = SERIALIZE_TYPE_STRING
        elif self.__is_maybe_dict_like(x[0]):
            base_type = SERIALIZE_TYPE_OBJECT
        else:
            raise ValueError("Cannot determine array element type for Python type {}".format(type(x[0])))

        array_type = base_type | SERIALIZE_FLAG_ARRAY
        self.__serialize_fixed_int(array_type, signed = False, byte_size = 1, include_type = False) #type
        self.__serialize_varint(len(x)) # length

        for elem in x:
            self.__dispatch_serialize_scalar(elem, include_type = False,
                signed_override = signed, byte_size_override = byte_size)

class Deserializer:
    def __init__(self, inf):
        if isinstance(inf, bytes):
            self.inf = io.BytesIO(inf)
        else:
            self.inf = inf

    def deserialize(self):
        assert(self.inf.read(len(PORTABLE_STORAGE_SIGNATURE)) == PORTABLE_STORAGE_SIGNATURE) 
        assert(self.inf.read(1)[0] == PORTABLE_STORAGE_FORMAT_VER)
        return self.__deserialize_section()

    def __deserialize_int(self, int_type):
        assert(int_type != 0)
        assert(int_type <= SERIALIZE_TYPE_UINT8)
        signed = int_type <= SERIALIZE_TYPE_INT8
        if int_type > SERIALIZE_TYPE_INT8:
            int_type -= 4
        assert(int_type <= SERIALIZE_TYPE_INT8)
        byte_size = 1 << (SERIALIZE_TYPE_INT8-int_type)
        int_bytes = self.inf.read(byte_size)
        assert(len(int_bytes) == byte_size)
        val = int.from_bytes(int_bytes, 'little')
        if signed:
            ones_mask = (1 << (byte_size*8)) - 1
            return val - ones_mask - 1
        else:
            return val

    def __deserialize_varint(self):
        int_bytes = self.inf.read(1)
        byte_size = 1 << (int_bytes[0] & PORTABLE_RAW_SIZE_MARK_MASK)
        if byte_size > 1:
            int_bytes += self.inf.read(byte_size - 1)
        assert(len(int_bytes) == byte_size)
        return int.from_bytes(int_bytes, 'little') >> 2

    def __deserialize_float(self):
        raise ValueError("Floating point numbers not supported yet")

    def __deserialize_string(self):
        length = self.__deserialize_varint()
        string = self.inf.read(length)
        assert(len(string) == length)
        return string

    def __deserialize_bool(self):
        b = self.inf.read(1)[0]
        return b != 0

    def __deserialize_section_key(self):
        key_length = self.__deserialize_int(SERIALIZE_TYPE_UINT8)
        key_bytes = self.inf.read(key_length)
        assert(len(key_bytes) == key_length)
        key_string = key_bytes.decode()
        assert(len(key_string) == key_length)
        return key_string

    def __deserialize_section_value(self):
        type_tag = self.__deserialize_int(SERIALIZE_TYPE_UINT8)
        is_array = (type_tag & SERIALIZE_FLAG_ARRAY) != 0
        base_type = type_tag & ~(SERIALIZE_FLAG_ARRAY)
        assert(base_type != 0)
        assert(base_type <= SERIALIZE_TYPE_OBJECT)
        if is_array:
            array_length = self.__deserialize_varint()
            val = []
            for _ in range(array_length):
                val.append(self.__deserialize_of_type(base_type))
            return val
        else:
            return self.__deserialize_of_type(base_type)

    def __deserialize_section(self):
        obj_length = self.__deserialize_varint()
        obj = {}
        for _ in range(obj_length):
            key = self.__deserialize_section_key()
            value = self.__deserialize_section_value()
            obj[key] = value
        return obj

    def __deserialize_of_type(self, serialize_type):
        assert(serialize_type > 0)
        assert(serialize_type <= SERIALIZE_TYPE_OBJECT)
        if serialize_type <= SERIALIZE_TYPE_UINT8:
            return self.__deserialize_int(serialize_type)
        elif serialize_type == SERIALIZE_TYPE_DOUBLE:
            return self.__deserialize_float()
        elif serialize_type == SERIALIZE_TYPE_STRING:
            return self.__deserialize_string()
        elif serialize_type == SERIALIZE_TYPE_BOOL:
            return self.__deserialize_bool()
        elif serialize_type == SERIALIZE_TYPE_OBJECT:
            return self.__deserialize_section()
        else:
            raise ValueError("Unrecognized serialize type: " + str(serialize_type))
