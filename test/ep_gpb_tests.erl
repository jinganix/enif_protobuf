%%% Copyright (C) 2010-2011  Tomas Abrahamsson
%%%
%%% Author: Tomas Abrahamsson <tab@lysator.liu.se>
%%%
%%% This library is free software; you can redistribute it and/or
%%% modify it under the terms of the GNU Lesser General Public
%%% License as published by the Free Software Foundation; either
%%% version 2.1 of the License, or (at your option) any later version.
%%%
%%% This library is distributed in the hope that it will be useful,
%%% but WITHOUT ANY WARRANTY; without even the implied warranty of
%%% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
%%% Lesser General Public License for more details.
%%%
%%% You should have received a copy of the GNU Lesser General Public
%%% License along with this library; if not, write to the Free Software
%%% Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
%%% MA  02110-1301  USA

%% origin: https://github.com/tomas-abrahamsson/gpb/blob/master/test/gpb_tests.erl

-module(ep_gpb_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-record(m1, {a}).
-record(m2, {b}).
-record(m4, {x, y}).

decode_msg(Bin, Name, Defs) ->
    decode_msg(Bin, Name, Defs, [{string_as_list, true}, {with_utf8, true}]).

decode_msg(Bin, Name, Defs, Opts) ->
    enif_protobuf:set_opts(Opts),
    enif_protobuf:load_cache(Defs),
    enif_protobuf:decode(Bin, Name).

encode_msg(Msg, Defs) ->
    encode_msg(Msg, Defs, [{string_as_list, true}, {with_utf8, true}]).

encode_msg(Msg, Defs, Opts) ->
    enif_protobuf:set_opts(Opts),
    enif_protobuf:load_cache(Defs),
    enif_protobuf:encode(Msg).

skipping_unknown_varint_field_test() ->
    #m1{a = 0} =
        decode_msg(<<32, 150, 1>>, %% field number 4 (not known), wire type = 0
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]).

skipping_unknown_length_delimited_field_test() ->
    #m1{a = 0} =
        decode_msg(<<34, 1, 1>>, %% field number 4 (not known), wire type = 2
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]).

skipping_unknown_64bit_field_test() ->
    #m1{a = 0} =
        decode_msg(<<33, 0, 0, 0, 0, 0, 0, 0, 0>>, %% field number 4, wire type = 1
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]).

skipping_unknown_32bit_field_test() ->
    #m1{a = 0} =
        decode_msg(<<37, 0, 0, 0, 0>>, %% field number 4, wire type = 5
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]).

skipping_with_oneof_test() ->
    Defs = [{{msg, m1}, [#gpb_oneof{
        name = a, rnum = #m1.a,
        fields = [#?gpb_field{name = a1, fnum = 1, rnum = #m1.a,
            type = int32, occurrence = optional,
            opts = []},
            #?gpb_field{name = a2, fnum = 2, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]}],
    #m1{a = undefined} = decode_msg(<<32, 150, 1>>, m1, Defs).

skipping_groups() ->
    DefsO = [{{msg, x1}, [Field1 = #?gpb_field{name = f1, fnum = 1, rnum = 2,
        type = uint32, occurrence = required,
        opts = []}]}],
    %% message with a group in a group
    DefsN = [{{msg, x1}, [Field1,
        #?gpb_field{name = g, fnum = 2, rnum = 3,
            type = {group, 'x1.g1'}, occurrence = optional,
            opts = []}]},
        {{group, 'x1.g1'}, [#?gpb_field{
            name = g1f, fnum = 3, rnum = 2,
            type = {group, 'x1.g2'}, occurrence = required,
            opts = []}]},
        {{group, 'x1.g2'}, [#?gpb_field{
            name = g2f, fnum = 4, rnum = 2,
            type = uint32, occurrence = required,
            opts = []}]}],
    X1 = encode_msg({x1, 38, {'x1.g1', {'x1.g2', 17}}}, DefsN),
    {x1, 38} = decode_msg(X1, x1, DefsO).

decode_msg_simple_occurrence_test() ->
    #m1{a = 0} =
        decode_msg(<<>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]),
    #m1{a = 150} =
        decode_msg(<<8, 150, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = required,
                opts = []}]}]),
    #m1{a = [150, 151]} =
        decode_msg(<<8, 150, 1, 8, 151, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = []}]}]),
    ok.

decode_msg_with_enum_field_test() ->
    #m1{a = v2} =
        decode_msg(<<8, 150, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v1, 100},
                    {v2, 150}]}]).

decode_msg_with_negative_enum_value_test() ->
    #m1{a = v2} =
        decode_msg(<<8, 254, 255, 255, 255, 15>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v1, 100},
                    {v2, -2}]}]),
    #m1{a = v2} =
        decode_msg(<<8, 254, 255, 255, 255, 255, 255, 255, 255, 255, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v1, 100},
                    {v2, -2}]}]).

decode_msg_with_enum_aliases_test() ->
    #m1{a = v1} =
        decode_msg(<<8, 100>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{option, allow_alias, true},
                    {v1, 100},
                    {v2, 100}]}]).

decode_unknown_enum_test() ->
    #m1{a = 4711} =
        decode_msg(<<8, 231, 36>>, m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v0, 0}]}]),
    #m1{a = [4711]} =
        decode_msg(<<8, 231, 36>>, m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = repeated,
                opts = []}]},
                {{enum, e}, [{v0, 0}]}]),
    #m1{a = [4711]} =
        decode_msg(<<10, 2, 231, 36>>, m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = repeated,
                opts = [packed]}]},
                {{enum, e}, [{v0, 0}]}]).

decode_msg_with_bool_field_test() ->
    #m1{a = true} =
        decode_msg(<<8, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]),
    #m1{a = false} =
        decode_msg(<<8, 0>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]).

decoding_float_test() ->
    %% Stole idea from the python test in google-protobuf:
    %% 1.125 is perfectly representable as a float (no rounding error).
    #m1{a = 1.125} =
        decode_msg(<<13, 0, 0, 144, 63>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = float, occurrence = required,
                opts = []}]}]).

decoding_double_test() ->
    #m1{a = 1.125} =
        decode_msg(<<9, 0, 0, 0, 0, 0, 0, 242, 63>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = double, occurrence = required,
                opts = []}]}]).

decode_msg_with_string_field_test() ->
    #m1{a = "abc\345\344\366" ++ [1022]} =
        decode_msg(<<10, 11,
            $a, $b, $c, $\303, $\245, $\303, $\244, $\303, $\266, $\317, $\276>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = string, occurrence = required,
                opts = []}]}]).

decode_msg_with_bytes_field_test() ->
    #m1{a = <<0, 0, 0, 0>>} =
        decode_msg(<<10, 4, 0, 0, 0, 0>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bytes, occurrence = required,
                opts = []}]}]).

decode_msg_with_sub_msg_field_test() ->
    #m1{a = #m2{b = 150}} =
        decode_msg(<<10, 3, 8, 150, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {msg, m2}, occurrence = required,
                opts = []}]},
                {{msg, m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
                    type = uint32, occurrence = required,
                    opts = []}]}]).

decode_msg_with_optional_nonpresent_sub_msg_field_test() ->
    #m1{a = undefined} =
        decode_msg(<<>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {msg, m2}, occurrence = optional,
                opts = []}]},
                {{msg, m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
                    type = uint32, occurrence = required,
                    opts = []}]}]).

decoding_zero_instances_of_packed_varints_test() ->
    %%    "A packed repeated field containing zero elements does not
    %%     appear in the encoded message."
    %%    -- http://code.google.com/apis/protocolbuffers/docs/encoding.html
    #m1{a = []} =
        decode_msg(<<>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = [packed]}]}]).

decoding_one_packed_chunk_of_varints_test() ->
    #m1{a = [3, 270, 86942]} =
        decode_msg(<<16#22,                 % tag (field number 4, wire type 2)
            16#06,                 % payload size (6 bytes)
            16#03,                 % first element (varint 3)
            16#8E, 16#02,          % second element (varint 270)
            16#9E, 16#a7, 16#05>>, % third element (varint 86942)
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 4, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = [packed]}]}]).

decoding_two_packed_chunks_of_varints_test() ->
    %%    "Note that although there's usually no reason to encode more
    %%     than one key-value pair for a packed repeated field, encoders
    %%     must be prepared to accept multiple key-value pairs. In this
    %%     case, the payloads should be concatenated. Each pair must
    %%     contain a whole number of elements."
    %%    -- http://code.google.com/apis/protocolbuffers/docs/encoding.html
    #m1{a = [3, 270, 86942, 4, 271, 86943]} =
        decode_msg(<<16#22, 16#06, 16#03, 16#8E, 16#02, 16#9E, 16#a7, 16#05,
            16#22, 16#06, 16#04, 16#8F, 16#02, 16#9F, 16#a7, 16#05>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 4, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = [packed]}]}]),
    ok.

decode_skips_nonpacked_fields_if_wiretype_mismatches_test() ->
    #m1{a = false} =
        decode_msg(<<9, %% 9 means wiretype=bits64 instead of expected varint
            0:64>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = optional,
                opts = []}]}]).

decode_skips_packed_fields_if_wiretype_mismatches_test() ->
    #m1{a = []} =
        decode_msg(<<9, %% 9 means wiretype=bits64 instead of expected varint
            0:64>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = repeated,
                opts = [packed]}]}]).

decode_of_field_fails_for_invalid_varints_test() ->
    ViMax64 = gpb:encode_varint(16#ffffFFFFffffFFFF),
    FDef = #?gpb_field{name = a, fnum = 1, rnum = #m1.a, type = bool,
        occurrence = required, opts = []},
    %% Verify fail on invalid field number + type
    ?assertError(_, decode_msg(<<255, ViMax64/binary, 1>>,
        m1,
        [{{msg, m1}, [FDef]}])),
    %% Verify fail on invalid field bools
    ?assertError(_, decode_msg(<<8, %% field num = 1, wire type = varint
        255, ViMax64/binary %% too many bits
    >>,
        m1,
        [{{msg, m1}, [FDef]}])),
    %% Verify fail on invalid field enums (enums are 32 bits signed,
    %% but might be sent over the wire as 64 bits signed, but are
    %% to be truncated to 32 bits before interpretation.
    ?assertError(_, decode_msg(<<8, %% field num = 1, wire type = varint
        255, ViMax64/binary %% too many bits
    >>,
        m1,
        [{{msg, m1}, [FDef#?gpb_field{type = {enum, e}}]},
            {{enum, e}, [{a, 1}]}])),
    %% Verify fail on invalid length, for length delimited field types
    [?assertError(_, decode_msg(<<10, %% field num = 1, wire type = len-delim
        255, ViMax64/binary, %% too many length bits
        1, 2, 3
    >>,
        m1,
        [{{msg, m1}, [FDef#?gpb_field{type = T}]},
            {{msg, m2}, [FDef]}]))
        || T <- [string, bytes, {msg, m2}]],
    %% Verify fail on invalid 32-bit varint field types
    [?assertError(_, decode_msg(<<8, %% field num = 1, wire type = varint
        255, ViMax64/binary %% too many bits
    >>,
        m1,
        [{{msg, m1}, [FDef#?gpb_field{type = T}]}]))
        || T <- [sint32, int32, uint32]],
    %% Verify fail on invalid 64-bit varint field types
    [?assertError(_, decode_msg(<<8, %% field num = 1, wire type = varint
        255, ViMax64/binary %% too many bits
    >>,
        m1,
        [{{msg, m1}, [FDef#?gpb_field{type = T}]}]))
        || T <- [sint64, int64, uint64]],
    %% Verify fail on invalid length for packed repeated field
    ?assertError(_, decode_msg(<<8, %% field num = 1, wire type = varint
        255, ViMax64/binary, %% too many length bits
        1, 1, 1
    >>,
        m1,
        [{{msg, m1}, [FDef#?gpb_field{occurrence = repeated,
            opts = [packed]}]}])),
    ok.

decoding_oneof_test() ->
    Defs = [{{msg, m1}, [#gpb_oneof{
        name = a, rnum = #m1.a,
        fields = [#?gpb_field{name = a1, fnum = 1, rnum = #m1.a,
            type = int32, occurrence = optional,
            opts = []},
            #?gpb_field{name = a2, fnum = 2, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]}],
    #m1{a = undefined} = decode_msg(<<>>, m1, Defs),
    #m1{a = {a1, 150}} = decode_msg(<<8, 150, 1>>, m1, Defs),
    #m1{a = {a2, 150}} = decode_msg(<<16, 150, 1>>, m1, Defs).

decoding_oneof_with_merge_test() ->
    Defs = [{{msg, m1}, [#gpb_oneof{
        name = a, rnum = #m1.a,
        fields = [#?gpb_field{name = x, fnum = 1, rnum = #m1.a,
            type = {msg, pb_m2}, occurrence = optional,
            opts = []},
            #?gpb_field{name = y, fnum = 2, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]},
        {{msg, pb_m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
            type = fixed32, occurrence = repeated}]}],
    B1 = encode_msg(#m1{a = {pb_m2, [1]}}, Defs),
    B2 = encode_msg(#m1{a = {pb_m2, [2]}}, Defs),
    B3 = encode_msg(#m1{a = {y, 150}}, Defs),

    %% Will get b=[1,2] since messages are merged
    #m1{a = {pb_m2, [1,2]}} = decode_msg(<<B1/binary, B2/binary>>, m1, Defs),
    %% Different oneof fields ==> no merge
    #m1{a = {y, 150}} = decode_msg(<<B1/binary, B3/binary>>, m1, Defs).

decoding_map_test() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = {map, string, fixed32},
        occurrence = repeated, opts = []}]}],
    #m1{a = Map} = decode_msg(<<
        %% first item: "x" => 17
        10,  %% map is a msg type item
        8,   %% sub msg len
        10,           %%% key-field (string)
        1, "x",        %%% len + key
        21,           %%% value-field (fixed32)
        17:32/little, %%% value
        %% second item: "y" => 18
        10, 8,
        10, 1, "y",      %% key
        21, 18:32/little %% value
    >>,
        m1,
        Defs),
    [{"x", 17}, {"y", 18}] = lists:sort(Map).

decoding_map_with_duplicate_keys_test() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = {map, string, fixed32},
        occurrence = repeated, opts = []}]}],
    #m1{a = Map} = decode_msg(
        %% A map with "x" => 16, (not to be included)
        %%            "x" => 17  (overrides "x" => 16)
        %%        and "y" => 18
        <<10, 8, 10, 1, "x", 21, 16:32/little,
            10, 8, 10, 1, "x", 21, 17:32/little,
            10, 8, 10, 1, "y", 21, 18:32/little>>,
        m1,
        Defs),
    [{"x", 17}, {"y", 18}] = lists:sort(Map).

decoding_map_with_keys_values_missing_should_get_type_defaults_test() ->
    %% map keys and values are optional, and when decoded, should
    %% get type-default values if missing.
    %% This is the case also with syntax="proto2".
    %% (a map with all values omitted could perhaps be regarded as a set,
    %% but we can't express that right now)
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 3, rnum = #m1.a,
        type = {map, string, uint32},
        occurrence = repeated, opts = []}]},
        {{msg, m1m}, % map<string,uint32> equivalent using bwd-compat msgs
            [#?gpb_field{name = a, fnum = 3, rnum = #m1.a,
                type = {msg, map_equiv},
                occurrence = repeated, opts = []}]},
        {{msg, map_equiv},
            [#?gpb_field{name = k,
                fnum = 1, rnum = 2,
                type = string, occurrence = optional, opts = []},
                #?gpb_field{name = v,
                    fnum = 2, rnum = 3,
                    type = uint32, occurrence = optional, opts = []}]}],
    B1m = encode_msg({m1m, [{map_equiv, "abc", 17},
        {map_equiv, undefined, 18},
        {map_equiv, "def", undefined},
        {map_equiv, undefined, undefined}]}, Defs),
    {m1, Map} = decode_msg(B1m, m1, Defs),
    [{"", _}, {"abc", 17}, {"def", 0}] = lists:sort(Map).

error_for_mapfield_with_missing_msgvalue_test() ->
    %% It is an error if a value is missing for a map<_,_> type field,
    %% and the type of the value is a message.
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 3, rnum = #m1.a,
        type = {map, string, {msg, m2}},
        occurrence = repeated, opts = []}]},
        {{msg, m2}, [#?gpb_field{name = b, fnum = 4, rnum = #m2.b, type = uint32,
            occurrence = required, opts = []}]},
        {{msg, m1m}, % map<string,uint32> equivalent using bwd-compat msgs
            [#?gpb_field{name = a, fnum = 3, rnum = #m1.a,
                type = {msg, map_equiv},
                occurrence = repeated, opts = []}]},
        {{msg, map_equiv},
            [#?gpb_field{name = k,
                fnum = 1, rnum = 2,
                type = string, occurrence = optional, opts = []},
                #?gpb_field{name = v,
                    fnum = 2, rnum = 3,
                    type = {msg, m2}, occurrence = optional, opts = []}]}],
    B1m = encode_msg({m1m, [{map_equiv, "abc", {m2, 17}},
        {map_equiv, "def", undefined}]}, Defs),
    ?assertError(_, decode_msg(B1m, m1, Defs)).

decode_packed_repeated_with_without_packed_test() ->
    {timeout, 10, fun decode_packed_repeated_with_without_packed_test_aux/0}.

decode_packed_repeated_with_without_packed_test_aux() ->
    %% "Protocol buffer parsers must be able to parse repeated fields
    %% that were compiled as packed as if they were not packed, and vice
    %% versa. [...]"
    %% -- https://developers.google.com/protocol-buffers/docs/encoding#packed
    %%
    %% Also: "Only repeated fields of primitive numeric types (types
    %% which use the varint, 32-bit, or 64-bit wire types) can be
    %% declared 'packed'."
    TypesWithValues = [{uint32, [3, 270, 86942]},
        {fixed32, [3, 270, 86942]},
        {float, [32.0, 0.0, 1.125]},
        {fixed64, [3, 270, 86942]}],
    [begin
        UF = #?gpb_field{name = a, fnum = 1, rnum = #m1.a, type = Type,
            occurrence = repeated, opts = []},
        PF = UF#?gpb_field{opts = [packed]},
        UDefs = [{{msg, m1}, [UF]}],
        PDefs = [{{msg, m1}, [PF]}],
        BU = encode_msg({m1, Values}, UDefs),
        BP = encode_msg({m1, Values}, PDefs),
        {m1, Values} = decode_msg(BU, m1, PDefs), % dec unpacked with packed
        {m1, Values} = decode_msg(BP, m1, UDefs)  % ... and vice versa
    end
        || {Type, Values} <- TypesWithValues],
    ok.

decoding_should_mask_to_bitlength_test() ->
    %% This is the result of decoding too large values
    %% with protobuf's generated Python and C++ output.
    %% gpb works like the C++ decoder.
    %%
    %%   Protobuf  Encoded   Python/C++
    %%   Type      value     decoded
    %%             on wire   value
    %%   ----------------------------------------------------------------
    %%   uint32    2^32      0
    %%   uint32    2^33-1    4294967295
    %%   uint32    2^33      0
    %%   uint32    2^34-1    4294967295
    %%   uint64    2^64      0
    %%   uint64    2^65-1    18446744073709551615
    %%   uint64    2^65      0
    %%   uint64    2^66-1    18446744073709551615
    %%
    %%   int32     2^32      0
    %%   int32     2^33-1    -1
    %%   int32     2^33      0
    %%   int32     2^34-1    -1
    %%   int64     2^64      0
    %%   int64     2^65-1    -1
    %%   int64     2^65      0
    %%   int64     2^66-1    -1
    %%
    %%   sint32    2^32      0
    %%   sint32    2^33-1    -2147483648
    %%   sint32    2^33      0
    %%   sint32    2^34-1    -2147483648
    %%   sint64    2^64      0
    %%   sint64    2^65-1    -9223372036854775808
    %%   sint64    2^65      0
    %%   sint64    2^66-1    -9223372036854775808
    %%
    %% Here are the various versions involved:
    %%   libprotoc 3.15.3
    %%   Python 3.9.2
    %%   gcc (Debian 10.2.1-6) 10.2.1 20210110
    %%
    [begin
        FNum = 1,
        Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = FNum, rnum = #m1.a,
            type = Type, occurrence = required,
            opts = []}]}],
        WType = gpb:encode_wiretype(Type),
        Encoded = iolist_to_binary(
            [gpb:encode_varint((FNum bsl 3) + WType),
                gpb:encode_varint(ValueToEncode)]),
        Env = {Type, ValueToEncode, Encoded, Defs},
        ?assertMatch({#m1{a = ExpectedDecoded}, _},
            {decode_msg(Encoded, m1, Defs), Env})
    end
        || {ExpectedDecoded, ValueToEncode, Type}
        <- [%% uint32,uint64
            {0, pow2(32), uint32},
            {max_uint(32), pow2(33) - 1, uint32},
            {0, pow2(64), uint64},
            {max_uint(64), pow2(65) - 1, uint64},
            %% int32,int64
            {0, pow2(32), int32},
            {-1, pow2(33) - 1, int32},
            {0, pow2(64), int64},
            {-1, pow2(65) - 1, int64},
            %% sint32,sint64
            {0, pow2(32), sint32},
            {min_sint(32), pow2(33) - 1, sint32},
            {0, pow2(64), sint64},
            {min_sint(64), pow2(65) - 1, sint64}]],
    ok.

max_uint(NumBits) -> pow2(NumBits) - 1.

min_sint(NumBits) -> -pow2(NumBits - 1).

pow2(NumBits) -> (1 bsl NumBits).

%% -------------------------------------------------------------

encode_required_varint_field_test() ->
    <<8, 150, 1>> =
        encode_msg(#m1{a = 150},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = required,
                opts = []}]}]).

encode_optional_varint_field_test() ->
    <<>> =
        encode_msg(#m1{a = undefined},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]).

encode_repeated_empty_field_test() ->
    <<>> =
        encode_msg(#m1{a = []},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = [packed]}]}]),
    <<>> =
        encode_msg(#m1{a = []},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = []}]}]).

encode_repeated_nonempty_field_test() ->
    <<10, 4, 150, 1, 151, 1>> =
        encode_msg(#m1{a = [150, 151]},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = [packed]}]}]),
    <<8, 150, 1, 8, 151, 1>> =
        encode_msg(#m1{a = [150, 151]},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = repeated,
                opts = []}]}]).

encode_msg_with_sub_msg_field_test() ->
    <<10, 3, 8, 150, 1>> =
        encode_msg(#m1{a = #m2{b = 150}},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {msg, m2},
                occurrence = required, opts = []}]},
                {{msg, m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
                    type = uint32, occurrence = required,
                    opts = []}]}]).

encode_msg_with_enum_field_test() ->
    <<8, 150, 1>> =
        encode_msg(#m1{a = v2},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v1, 100},
                    {v2, 150}]}]).

negative_int32_types_test() ->
    %% Interop with other protobuf implementations:
    %%
    %% A value of -1 for an int32 can be encoded as:
    %% <<8,255,255,255,255,15>>, but the Google protobuf (C++) encodes
    %% it as <<8,255,255,255,255,255,255,255,255,255,1>>, ie using 64
    %% bits.
    %%
    %% Google protobuf (C++) decodes both octet sequences to -1, but
    %% there are other protobuf implementations that decode to
    %% different values. So for interop, encode using 64 bits,
    %% on decoding, accept both 32 and 64 bits.
    %%
    %% Enums are encoded as int32.
    %%
    %% The sint32 type is different.
    %%
    Defs = [{{msg, m4}, [#?gpb_field{name = x, fnum = 1, rnum = #m4.x,
        type = int32, occurrence = optional,
        opts = []},
        #?gpb_field{name = y, fnum = 2, rnum = #m4.y,
            type = {enum, e}, occurrence = optional,
            opts = []}]},
        {{enum, e}, [{z, 0},
            {n2, -2},
            {p2, 2}]}],
    Minus2As64Bits = <<254, 255, 255, 255, 255, 255, 255, 255, 255, 1>>,
    Minus2As32Bits = <<254, 255, 255, 255, 15>>,
    X_Minus2As64Bits = <<8, Minus2As64Bits/binary>>,
    X_Minus2As32Bits = <<8, Minus2As32Bits/binary>>,
    Y_Minus2As64Bits = <<16, Minus2As64Bits/binary>>,
    Y_Minus2As32Bits = <<16, Minus2As32Bits/binary>>,

    %% Check int32
    X_Minus2As64Bits = encode_msg(#m4{x = -2}, Defs),
    #m4{x = -2} = decode_msg(X_Minus2As64Bits, m4, Defs),
    #m4{x = -2} = decode_msg(X_Minus2As32Bits, m4, Defs),

    %% Check enums
    Y_Minus2As64Bits = encode_msg(#m4{y = n2}, Defs),
    #m4{y = n2} = decode_msg(Y_Minus2As64Bits, m4, Defs),
    #m4{y = n2} = decode_msg(Y_Minus2As32Bits, m4, Defs),
    ok.

encode_msg_with_enum_aliases_test() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = {enum, e}, occurrence = required,
        opts = []}]},
        {{enum, e}, [{option, allow_alias, true},
            {v1, 100},
            {v2, 100}]}],
    <<8, 100>> = encode_msg(#m1{a = v1}, Defs),
    <<8, 100>> = encode_msg(#m1{a = v2}, Defs).

encode_unknown_enum_test() ->
    <<8, 231, 36>> =
        encode_msg(#m1{a = 4711},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = required,
                opts = []}]},
                {{enum, e}, [{v0, 0}]}]),
    <<8, 231, 36>> =
        encode_msg(#m1{a = [4711]},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = repeated,
                opts = []}]},
                {{enum, e}, [{v0, 0}]}]),
    <<10, 2, 231, 36>> =
        encode_msg(#m1{a = [4711]},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = {enum, e}, occurrence = repeated,
                opts = [packed]}]},
                {{enum, e}, [{v0, 0}]}]).

encode_msg_with_bool_field_test() ->
    <<8, 1>> =
        encode_msg(#m1{a = true},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]),
    <<8, 1>> =
        encode_msg(#m1{a = 1},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]),
    <<8, 0>> =
        encode_msg(#m1{a = false},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]),
    <<8, 0>> =
        encode_msg(#m1{a = 0},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = bool, occurrence = required,
                opts = []}]}]).

encode_float_test() ->
    %% Stole idea from the python test in google-protobuf:
    %% 1.125 is perfectly representable as a float (no rounding error).
    <<13, 0, 0, 144, 63>> =
        encode_msg(#m1{a = 1.125},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = float, occurrence = required,
                opts = []}]}]).

encode_packed_repeated_bools_test() ->
    <<16#22, 1, 1>> =
        encode_msg(#m1{a = [true]},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 4, rnum = #m1.a,
                type = bool, occurrence = repeated,
                opts = [packed]}]}]).

decode_packed_repeated_bools_test() ->
    #m1{a = [true]} =
        decode_msg(<<16#22, 1, 1>>,
            m1,
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 4, rnum = #m1.a,
                type = bool, occurrence = repeated,
                opts = [packed]}]}]).

encode_double_test() ->
    <<9, 0, 0, 0, 0, 0, 0, 242, 63>> =
        encode_msg(#m1{a = 1.125},
            [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
                type = double, occurrence = required,
                opts = []}]}]).

encode_numbers_as_floats_doubles_test() ->
    %% 16 is an integer which is also perfectly representable as an IEEE float
    %% (no rounding error)
    DefsF = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = float, occurrence = required,
        opts = []}]}],
    DefsD = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = double, occurrence = required,
        opts = []}]}],
    DefsFP = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = float, occurrence = repeated,
        opts = [packed]}]}],
    DefsDP = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = double, occurrence = repeated,
        opts = [packed]}]}],
    <<13, 0:16, 128, 65>> = encode_msg(#m1{a = 16}, DefsF),
    <<13, 0:16, 128, 65>> = encode_msg(#m1{a = 16.0}, DefsF),
    <<9, 0:48, 48, 64>> = encode_msg(#m1{a = 16}, DefsD),
    <<9, 0:48, 48, 64>> = encode_msg(#m1{a = 16.0}, DefsD),
    <<10, 4, 0:16, 128, 65>> = encode_msg(#m1{a = [16]}, DefsFP),
    <<10, 4, 0:16, 128, 65>> = encode_msg(#m1{a = [16.0]}, DefsFP),
    <<10, 8, 0:48, 48, 64>> = encode_msg(#m1{a = [16]}, DefsDP),
    <<10, 8, 0:48, 48, 64>> = encode_msg(#m1{a = [16.0]}, DefsDP),
    ok.

'+inf,-inf,nan_double_test'() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = double, occurrence = repeated,
        opts = [packed]}]},
        {{msg, m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
            type = double, occurrence = required,
            opts = []}]}],
    Msg = #m1{a = [infinity, '-infinity', nan]},
    <<10, 24,
        0:48, 16#f0, 16#7f,
        0:48, 16#f0, 16#ff,
        0:48, 16#f8, 16#7f>> = B = encode_msg(Msg, Defs),
    Msg = decode_msg(B, m1, Defs),
    Msg2 = #m2{b = infinity},
    <<9, 0:48, 16#f0, 16#7f>> = B2 = encode_msg(Msg2, Defs),
    Msg2 = decode_msg(B2, m2, Defs),
    %% check that decode also recognizes other forms of NaN:
    %% Both sets of fraction bits /= 0, and also sign bit /= 0:
    B3 = <<10, 8, 4711:48, 16#ff, 16#ff>>,
    #m1{a = [nan]} = decode_msg(B3, m1, Defs),
    B4 = <<9, 4711:48, 16#ff, 16#ff>>,
    #m2{b = nan} = decode_msg(B4, m2, Defs).

'+inf,-inf,nan_float_test'() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = float, occurrence = repeated,
        opts = [packed]}]},
        {{msg, m2}, [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
            type = float, occurrence = required,
            opts = []}]}],
    Msg = #m1{a = [infinity, '-infinity', nan]},
    <<10, 12,
        0:16, 16#80, 16#7f,
        0:16, 16#80, 16#ff,
        0:16, 16#c0, 16#7f>> = B = encode_msg(Msg, Defs),
    Msg = decode_msg(B, m1, Defs),
    Msg2 = #m2{b = infinity},
    <<13, 0:16, 16#80, 16#7f>> = B2 = encode_msg(Msg2, Defs),
    Msg2 = decode_msg(B2, m2, Defs),
    %% check that decode also recognizes other forms of NaN:
    %% Both sets of fraction bits /= 0, and also sign bit /= 0:
    B3 = <<10, 4, 4711:16, 16#ff, 16#ff>>,
    #m1{a = [nan]} = decode_msg(B3, m1, Defs),
    B4 = <<13, 4711:16, 16#ff, 16#ff>>,
    #m2{b = nan} = decode_msg(B4, m2, Defs).

encode_oneof_test() ->
    Defs = [{{msg, m1}, [#gpb_oneof{
        name = a, rnum = #m1.a,
        fields = [#?gpb_field{name = a1, fnum = 1, rnum = #m1.a,
            type = int32, occurrence = optional,
            opts = []},
            #?gpb_field{name = a2, fnum = 2, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []}]}]}],
    <<>> = encode_msg(#m1{a = undefined}, Defs),
    <<8, 150, 1>> = encode_msg(#m1{a = {a1, 150}}, Defs),
    <<16, 150, 1>> = encode_msg(#m1{a = {a2, 150}}, Defs).

encode_map_test() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
        type = {map, string, fixed32},
        occurrence = repeated, opts = []}]}],
    <<10, 8,
        10, 1, "x",         %% key
        21, 17:32/little, %% value
        10, 8,
        10, 1, "y",       %% key
        21, 18:32/little  %% value
    >> = encode_msg(#m1{a = [{"x", 17}, {"y", 18}]}, Defs),

    %% Map items are present even if they have type-defaults
    %% even if syntax="proto3"
    P3Defs = [{syntax, "proto3"},
        {proto3_msgs, [m1]},
        {{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = {map, string, uint32},
            occurrence = repeated, opts = []}]}],
    <<10, 4, 10, 0, 16, 0>> = encode_msg(#m1{a = [{"", 0}]}, P3Defs),
    %%      ^^^^  ^^^^
    %%      Key   Value

    %% Map fields are not packed, even if proto3
    %% (the spec says it is equivalent to repeated MapFieldItem x = 17;
    %% and in proto3, repeated primitive fields are packed by default.
    %% Now this isn't a packed primitive field, but check just in case.
    <<10, 5, 10, 1, 97, 16, 1,    % "a" => 1
        10, 5, 10, 1, 98, 16, 2>> = % "b" => 2
    encode_msg(#m1{a = [{"a", 1}, {"b", 2}]}, P3Defs).

encode_proto3_unicode_strings_test() ->
    P3Defs = [{syntax, "proto3"},
        {proto3_msgs, [m1, m2]},
        {{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = {map, string, string},
            occurrence = repeated, opts = []}]},
        {{msg, m2}, [#?gpb_field{name = a, fnum = 1, rnum = #m2.b,
            type = string,
            occurrence = defaulty, opts = []}]}],
    Smiley1 = [16#1f631],
    Smiley1B = <<240, 159, 152, 177>> = unicode:characters_to_binary(Smiley1),
    Smiley2 = [16#1f628],
    Smiley2B = <<240, 159, 152, 168>> = unicode:characters_to_binary(Smiley2),
    <<10, 12,
        10, 4, Smiley1B:4/binary,
        18, 4, Smiley2B:4/binary>> =
        encode_msg(#m1{a = [{Smiley1, Smiley2}]}, P3Defs, [{string_as_list, false}]),

    <<10, 4, Smiley1B:4/binary>> = encode_msg(#m2{b = [Smiley1]}, P3Defs, [{string_as_list, false}]).

encode_proto3_various_empty_string_test() ->
    %% With iolists for encoding, there are many ways to specify an empty
    %% string.  In proto3, they should encode to nothing.
    P3Defs = [{syntax, "proto3"},
        {proto3_msgs, [m1, m2]},
        {{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = {map, uint32, string},
            occurrence = repeated, opts = []}]},
        {{msg, m2}, [#?gpb_field{name = a, fnum = 1, rnum = #m2.b,
            type = string,
            occurrence = defaulty, opts = []}]}],
    SuperEmpty = [<<>>, [[[<<>>, <<>>]]]],
    <<10, 4, 8, 1, 18, 0,
        10, 4, 8, 2, 18, 0,
        10, 6, 8, 3, 18, 2, 97, 98,
        10, 5, 8, 4, 18, 1, 65>> = encode_msg(#m1{a = [{1, <<>>}, {2, SuperEmpty},
        {3, [97, [<<98>>]]}, {4, <<65>>}]},
        P3Defs, [{string_as_list, false}]),
    <<>> = encode_msg(#m2{b = ""}, P3Defs, [{string_as_list, false}]),
    <<>> = encode_msg(#m2{b = <<>>}, P3Defs, [{string_as_list, false}]),
    <<>> = encode_msg(#m2{b = SuperEmpty}, P3Defs, [{string_as_list, false}]),
    <<10, 2, 97, 98>> = encode_msg(#m2{b = [97, [<<98>>]]}, P3Defs, [{string_as_list, false}]),
    <<10, 1, 65>> = encode_msg(#m2{b = <<65>>}, P3Defs, [{string_as_list, false}]).

encode_proto3_various_empty_sequence_of_bytes_test() ->
    %% With iolists for encoding, there are many ways to specify an empty
    %% binary.  In proto3, they should encode to nothing.
    P3Defs = [{syntax, "proto3"},
        {proto3_msgs, [m1, m2]},
        {{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = {map, uint32, bytes},
            occurrence = repeated, opts = []}]},
        {{msg, m2}, [#?gpb_field{name = a, fnum = 1, rnum = #m2.b,
            type = bytes,
            occurrence = defaulty, opts = []}]}],
    SuperEmpty = [<<>>, [[[<<>>, <<>>]]]],
    <<10, 4, 8, 1, 18, 0,
        10, 4, 8, 2, 18, 0,
        10, 6, 8, 3, 18, 2, 97, 98,
        10, 5, 8, 4, 18, 1, 65>> = encode_msg(#m1{a = [{1, <<>>}, {2, SuperEmpty},
        {3, [97, [<<98>>]]}, {4, <<65>>}]},
        P3Defs),
    <<>> = encode_msg(#m2{b = <<>>}, P3Defs),
    <<>> = encode_msg(#m2{b = ""}, P3Defs),
    <<>> = encode_msg(#m2{b = SuperEmpty}, P3Defs),
    <<10, 2, 97, 98>> = encode_msg(#m2{b = [97, [<<98>>]]}, P3Defs),
    <<10, 1, 65>> = encode_msg(#m2{b = <<65>>}, P3Defs).

proto3_type_default_values_never_serialized_test() ->
    %% "... if a scalar message field is set to its default, the value
    %% will not be serialized on the wire."
    %% -- https://developers.google.com/protocol-buffers/docs/proto3#default
    %%
    %% Assuming for oneof it means default = undefined (but I couldn't
    %% find any mention of this in the official protobuf docs)
    Defs = [{syntax, "proto3"},
        {proto3_msgs, [m, s]},
        {{enum, e}, [{e0, 0}, {e1, 1}]},
        {{msg, s}, % submsg
            [#?gpb_field{name = a, fnum = 1, rnum = 2, type = string,
                occurrence = defaulty, opts = []}]},
        {{msg, m},
            [#?gpb_field{name = a, fnum = 1, rnum = 2, type = string,
                occurrence = defaulty, opts = []},
                #?gpb_field{name = b, fnum = 2, rnum = 3, type = bytes,
                    occurrence = defaulty, opts = []},
                #?gpb_field{name = c, fnum = 3, rnum = 4, type = bool,
                    occurrence = defaulty, opts = []},
                #?gpb_field{name = d, fnum = 4, rnum = 5, type = uint32,
                    occurrence = defaulty, opts = []},
                #?gpb_field{name = e, fnum = 5, rnum = 6, type = float,
                    occurrence = defaulty, opts = []},
                #?gpb_field{name = f, fnum = 6, rnum = 7, type = double,
                    occurrence = defaulty, opts = []},
                #?gpb_field{name = g, fnum = 7, rnum = 8, type = {msg, s},
                    occurrence = defaulty, opts = []},
                #gpb_oneof{
                    name = h, rnum = 9,
                    fields = [#?gpb_field{name = ha, fnum = 8, rnum = 9, type = uint32,
                        occurrence = optional, opts = []}]}]}],
    Msg = {m, "", <<>>, false, % string, bytes, bool
        0,               % numeric types
        0.0, 0.0,        % float, double
        undefined,       % submsg
        undefined},      % oneof
    <<>> = encode_msg(Msg, Defs),
    Msg = decode_msg(<<>>, m, Defs).

proto3_type_default_values_never_serialized_for_enums_test() ->
    Defs = [{syntax, "proto3"},
        {proto3_msgs, [m]},
        {{enum, e}, [{e0, 0}, {e1, 1}]},
        {{msg, m}, [#?gpb_field{name = a, fnum = 1, rnum = 2, type = {enum, e},
            occurrence = defaulty, opts = []}]}],
    <<>> = encode_msg({m, e0}, Defs),
    <<>> = encode_msg({m, 0}, Defs), % when given as integer
    {m, e0} = decode_msg(<<>>, m, Defs).

proto3_optional_test() ->
    Defs = [{proto_defs_version, 2},
        {syntax, "proto3"},
        {proto3_msgs, [m1]},
        {{msg, m1}, [#?gpb_field{name = a, fnum = 1, rnum = 2, type = uint32,
            occurrence = optional, opts = []}]}],
    B1 = encode_msg({m1, 0}, Defs),
    ?assert(<<>> /= B1), % Must be sent even if the value = type-default
    #m1{a = 0} = decode_msg(B1, m1, Defs),
    <<>> = B2 = encode_msg({m1, undefined}, Defs),
    %% Expect undefined, instead of type-default, which would have
    %% been the case with occurrence = defaulty:
    #m1{a = undefined} = decode_msg(B2, m1, Defs),
    ok.

encode_decode_required_group() ->
    %% message m1 {
    %%   required group g = 30 {
    %%     required fixed32 gf = 35;
    %%   }
    %% }
    Defs = [{{msg, m1}, [#?gpb_field{name = g, fnum = 30, rnum = 2,
        type = {group, 'm1.g'},
        occurrence = required, opts = []}]},
        {{group, 'm1.g'}, [#?gpb_field{name = gf, fnum = 35, rnum = 2,
            type = fixed32,
            occurrence = required, opts = []}]}],
    M = {m1, {'m1.g', 17}},
    D = "f3 01 9d 02   11 00 00 00 f4 01",
    %%   ^^^^^ ^^^^^   ^^^^^^^^^^^ ^^^^^
    %%   GROUP TAG     17          GROUP
    %%   START FIXED32             END
    B = hexundump(D),
    B = encode_msg(M, Defs),
    M = decode_msg(B, m1, Defs).

encode_decode_repeated_and_optional_group() ->
    Defs = [{{msg, m1}, [#?gpb_field{name = g, fnum = 31, rnum = 2,
        type = {group, 'm1.g'},
        occurrence = repeated, opts = []},
        #?gpb_field{name = h, fnum = 32, rnum = 3,
            type = {group, 'm1.h'},
            occurrence = optional, opts = []}]},
        {{group, 'm1.g'}, [#?gpb_field{name = gf, fnum = 36, rnum = 2,
            type = fixed32,
            occurrence = required, opts = []}]},
        {{group, 'm1.h'}, [#?gpb_field{name = hf, fnum = 37, rnum = 2,
            type = fixed32,
            occurrence = required, opts = []}]}],
    M1 = {m1, [{'m1.g', 17}, {'m1.g', 18}], undefined},
    B1 = hexundump("fb 01 a5 02   11 00 00 00 fc 01"
    "fb 01 a5 02   12 00 00 00 fc 01"),
    %%              ^^^^^ ^^^^^   ^^^^^^^^^^^ ^^^^^
    %%              GROUP TAG     17,18       GROUP
    %%              START FIXED32             END
    B1 = encode_msg(M1, Defs),
    M1 = decode_msg(B1, m1, Defs),
    %% Now with merge
    M2 = {m1, [], {'m1.h', 99}},
    B21 = hexundump("83 02 ad 02   11 00 00 00 84 02"), % 17 to be over-merged
    B22 = hexundump("83 02 ad 02   63 00 00 00 84 02"), % 99 to replace
    B22 = encode_msg(M2, Defs),
    M2 = decode_msg(<<B21/binary, B22/binary>>, m1, Defs).

%%hexdump(B) ->
%%    string:to_lower(lists:concat([integer_to_list(C,16) || <<C:4>> <= B])).
hexundump(S) ->
    <<<<(list_to_integer([C], 16)):4>> || C <- S, is_hex_digit(C)>>.
is_hex_digit(D) when $0 =< D, D =< $9 -> true;
is_hex_digit(D) when $a =< D, D =< $f -> true;
is_hex_digit(D) when $A =< D, D =< $F -> true;
is_hex_digit(_) -> false.

encode_decode_basic_unknowns() ->
    Field1 = #?gpb_field{name = a, fnum = 1, rnum = 2, type = string,
        occurrence = optional,
        opts = []},
    FieldU = #?gpb_field{name = '$unknown', fnum = undefined, rnum = 0,
        type = unknown, occurrence = repeated,
        opts = []},
    Defs1 = [{{msg, msg}, [Field1,
        FieldU#?gpb_field{rnum = 3}]}],
    Defs2 = [{{msg, msg}, [Field1,
        %% varint:
        #?gpb_field{name = n2, fnum = 2, rnum = 3, type = int32,
            occurrence = optional,
            opts = []},
        %% 64 bits
        #?gpb_field{name = n3, fnum = 3, rnum = 4, type = fixed64,
            occurrence = optional,
            opts = []},
        %% length-delimited
        #?gpb_field{name = n4, fnum = 4, rnum = 5, type = bytes,
            occurrence = optional,
            opts = []},
        %% group
        #?gpb_field{name = n5, fnum = 5, rnum = 6,
            type = {group, gr},
            occurrence = optional,
            opts = []},
        %% 32 bits
        #?gpb_field{name = n6, fnum = 6, rnum = 7, type = fixed32,
            occurrence = optional,
            opts = []},
        %% --
        FieldU#?gpb_field{rnum = 8}]},
        {{group, gr}, [#?gpb_field{name = a, fnum = 10, rnum = 2, type = int32,
            occurrence = optional,
            opts = []}]}],
    Msg0 = {msg, "abc", 2, 3, <<4, 4>>, {gr, 5}, 6, []},
    E1 = encode_msg(Msg0, Defs2),
    %% Decode with Defs1, unknown fields should end up in '$unknown'
    %% Then encode this with the unknowns
    {msg, "abc", [_, _, _, _, _] = Unknowns} = Msg1 = decode_msg(E1, msg, Defs1),
    [{varint, 2, 2},
        {fixed64, 3, 3},
        {length_delimited, 4, <<4, 4>>},
        {group, 5, [{varint, 10, 5}]},
        {fixed32, 6, 6}] = Unknowns,
    E2 = encode_msg(Msg1, Defs1),
    %% decode with richer defs, should get back orig:
    D2 = decode_msg(E2, msg, Defs2),
    ?assertEqual(Msg0, D2).

encode_decode_repeated_unknowns() ->
    Field1 = #?gpb_field{name = a, fnum = 1, rnum = 2, type = string,
        occurrence = optional,
        opts = []},
    FieldU = #?gpb_field{name = '$unknown', fnum = undefined, rnum = 0,
        type = unknown, occurrence = repeated,
        opts = []},
    Defs1 = [{{msg, msg}, [Field1,
        FieldU#?gpb_field{rnum = 3}]}],
    Defs2 = [{{msg, msg}, [Field1,
        %% varint:
        #?gpb_field{name = nr, fnum = 2, rnum = 3, type = int32,
            occurrence = repeated,
            opts = []},
        %% --
        FieldU#?gpb_field{rnum = 4}]}],
    Msg0 = {msg, "abc", [17, 18, 19, 20], []},
    E1 = encode_msg(Msg0, Defs2),
    %% A repeated non-'packed' will be encoded as several fields, one
    %% for each element in the repeated sequence, so expect 4 unknowns:
    {msg, "abc", [_, _, _, _] = Unknowns} = Msg1 = decode_msg(E1, msg, Defs1),
    %% The order of the unknowns is important, check that:
    [{varint, 2, 17}, {varint, 2, 18}, {varint, 2, 19}, {varint, 2, 20}] = Unknowns,
    E2 = encode_msg(Msg1, Defs1),
    D2 = decode_msg(E2, msg, Defs2),
    ?assertEqual(Msg0, D2).

-ifndef(NO_HAVE_MAPS).
msg_to_from_map() ->
    %% map<_,_> messages
    MtDefs = [{{msg, m1},
        [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = {map, string, fixed32},
            occurrence = repeated, opts = []}]}],
    Msg10 = #m1{a = [{"a", 1}, {"b", 2}]},
    #{a := #{"a" := 1, "b" := 2}} = Map10 =
        gpb:msg_to_map(Msg10, MtDefs, []),
    Msg10 = sort_elem2(gpb:msg_from_map(Map10, m1, MtDefs, [])),
    Msg10 = sort_elem2(gpb:msg_from_map(#{a => Msg10#m1.a}, m1, MtDefs,
        [{mapfields_as_maps, false}])),

    %% Oneof
    OoDefs = [{{msg, m1},
        [#gpb_oneof{
            name = a, rnum = #m1.a,
            fields = [#?gpb_field{name = a1, fnum = 1, rnum = #m1.a,
                type = int32, occurrence = optional,
                opts = []},
                #?gpb_field{name = a2, fnum = 2, rnum = #m1.a,
                    type = int32, occurrence = optional,
                    opts = []}]}]}],

    Msg20 = #m1{a = {a1, 10}},
    #{a := {a1, 10}} = Map20 =
        gpb:msg_to_map(Msg20, OoDefs, []),
    Msg20 = gpb:msg_from_map(Map20, m1, OoDefs, []),
    %% Flat oneof
    #{a1 := 10} = Map21 =
        gpb:msg_to_map(Msg20, OoDefs, [{maps_oneof, flat}]),
    Msg20 = gpb:msg_from_map(Map21, m1, OoDefs, [{maps_oneof, flat}]),

    %% Unset
    Msg22 = #m1{a = undefined},
    ?assertEqual(#{}, gpb:msg_to_map(Msg22, OoDefs, [])),
    ?assertEqual(#{}, gpb:msg_to_map(Msg22, OoDefs, [{maps_oneof, flat}])),
    Msg22 = gpb:msg_from_map(#{}, m1, OoDefs, []),
    Msg22 = gpb:msg_from_map(#{}, m1, OoDefs, [{maps_oneof, flat}]),

    %% Repeated
    RpDefs = [{{msg, m1},
        [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = uint32,
            occurrence = repeated, opts = []}]}],
    Msg30 = #m1{a = [1, 2, 3]},
    #{a := [1, 2, 3]} = Map30 = gpb:msg_to_map(Msg30, RpDefs, []),
    Msg30 = gpb:msg_from_map(Map30, m1, RpDefs, []),

    %% Required
    RqDefs = [{{msg, m1},
        [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = uint32,
            occurrence = required, opts = []}]}],
    Msg40 = #m1{a = 4711},
    #{a := 4711} = Map40 = gpb:msg_to_map(Msg40, RqDefs, []),
    Msg40 = gpb:msg_from_map(Map40, m1, RqDefs, []),

    %% Optional (proto2 and proto3)
    O2Defs = [{{msg, m1},
        [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = uint32,
            occurrence = optional, opts = []}]}],
    O3Defs = [{{msg, m1},
        [#?gpb_field{name = a, fnum = 1, rnum = #m1.a,
            type = uint32,
            occurrence = defaulty, opts = []}]},
        {proto3_msgs, [m1]}],
    OptOmitted = {maps_unset_optional, omitted},
    OptPresentUndefined = {maps_unset_optional, present_undefined},
    Msg51 = #m1{a = 4711},
    Msg52 = #m1{a = undefined},
    #{a := 4711} = Map51 = gpb:msg_to_map(Msg51, O2Defs, []),
    Msg51 = gpb:msg_from_map(Map51, m1, O2Defs, []),
    Msg51 = gpb:msg_from_map(Map51, m1, O2Defs,
        []),
    ?assertEqual(#{}, gpb:msg_to_map(Msg52, O2Defs, [])),
    ?assertEqual(#{}, gpb:msg_to_map(Msg52, O2Defs, [OptOmitted])),
    #{a := undefined} = Map52 =
        gpb:msg_to_map(Msg52, O2Defs, [OptPresentUndefined]),
    Msg52 = gpb:msg_from_map(#{}, m1, O2Defs, []),
    Msg52 = gpb:msg_from_map(Map52, m1, O2Defs, [OptPresentUndefined]),

    O3Map = gpb:msg_to_map({m1, undefined}, O3Defs, []),
    0 = maps:size(O3Map),
    #m1{a = undefined} = gpb:msg_from_map(#{}, m1, O3Defs, []),
    #m1{a = undefined} = gpb:msg_from_map(#{}, m1, O3Defs, [OptOmitted]),
    #m1{a = undefined} = gpb:msg_from_map(#{a => undefined}, m1, O3Defs,
        [OptPresentUndefined]),

    %% Submessages are to get converted recursively
    SmDefs = [{{msg, m},
        [#?gpb_field{name = mt, fnum = 1, rnum = 2,
            type = {map, uint32, {msg, m2}},
            occurrence = repeated, opts = []},
            #?gpb_field{name = rp, fnum = 1, rnum = 2,
                type = {msg, m2},
                occurrence = repeated, opts = []},
            #?gpb_field{name = op, fnum = 1, rnum = 3,
                type = {msg, m2},
                occurrence = optional, opts = []},
            #?gpb_field{name = rq, fnum = 1, rnum = 4,
                type = {msg, m2},
                occurrence = required, opts = []},
            #gpb_oneof{name = c, rnum = 5,
                fields = [#?gpb_field{name = a1, fnum = 1, rnum = 5,
                    type = {msg, m2},
                    occurrence = optional,
                    opts = []}]}]},
        {{msg, m2},
            [#?gpb_field{name = b, fnum = 1, rnum = #m2.b,
                type = uint32,
                occurrence = optional, opts = []}]}],
    Msg61 = {m, [{"a", #m2{b = 11}}],
        [#m2{b = 12}], #m2{b = 13}, #m2{b = 14}, {a1, #m2{b = 15}}},
    #{mt := #{"a" := #{b := 11}},
        rp := [#{b := 12}],
        op := #{b := 13},
        rq := #{b := 14},
        c := {a1, #{b := 15}}} = Map61 = gpb:msg_to_map(Msg61, SmDefs, []),
    Msg61 = gpb:msg_from_map(Map61, m, SmDefs, []),
    #{mt := #{"a" := #{b := 11}},
        rp := [#{b := 12}],
        op := #{b := 13},
        rq := #{b := 14},
        a1 := #{b := 15}} = Map62 = gpb:msg_to_map(Msg61, SmDefs,
        [{maps_oneof, flat}]),
    Msg61 = gpb:msg_from_map(Map62, m, SmDefs, [{maps_oneof, flat}]),
    ok.

sort_elem2(Tuple) ->
    setelement(2, Tuple, lists:sort(element(2, Tuple))).
-endif. % -ifndef(NO_HAVE_MAPS).
