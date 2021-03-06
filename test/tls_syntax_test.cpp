#include "common.h"
#include "tls_syntax.h"
#include <gtest/gtest.h>

using namespace mls;

// An enum to test enum encoding, and as a type for variants
enum struct IntSelector : uint16_t
{
  uint8 = 0xAAAA,
  uint16 = 0xBBBB,
};

struct Uint8
{
  uint8_t value;
  static const IntSelector type;
  TLS_SERIALIZABLE(value)
};

const IntSelector Uint8::type = IntSelector::uint8;

struct Uint16
{
  uint16_t value;
  static const IntSelector type;
  TLS_SERIALIZABLE(value)
};

const IntSelector Uint16::type = IntSelector::uint16;

// A struct to test struct encoding and traits
struct ExampleStruct
{
  uint16_t a;
  std::array<uint32_t, 4> b;
  std::optional<uint8_t> c;
  std::vector<uint8_t> d;
  std::variant<Uint8, Uint16> e;

  TLS_SERIALIZABLE(a, b, c, d, e)
  TLS_TRAITS(tls::pass,
             tls::pass,
             tls::pass,
             tls::vector<2>,
             tls::variant<IntSelector>)
};

bool
operator==(const ExampleStruct& lhs, const ExampleStruct& rhs)
{
  return (lhs.a == rhs.a) && (lhs.b == rhs.b) && (lhs.c == rhs.c);
}

// Known-answer tests
class TLSSyntaxTest : public ::testing::Test
{
protected:
  const bool val_bool = true;
  const bytes enc_bool = from_hex("01");

  const uint8_t val_uint8{ 0x11 };
  const bytes enc_uint8 = from_hex("11");

  const uint16_t val_uint16{ 0x2222 };
  const bytes enc_uint16 = from_hex("2222");

  const uint32_t val_uint32{ 0x44444444 };
  const bytes enc_uint32 = from_hex("44444444");

  const uint64_t val_uint64{ 0x8888888888888888 };
  const bytes enc_uint64 = from_hex("8888888888888888");

  const std::array<uint16_t, 4> val_array{ 1, 2, 3, 4 };
  const bytes enc_array = from_hex("0001000200030004");

  const ExampleStruct val_struct{
    0x1111,
    { 0x22222222, 0x33333333, 0x44444444, 0x55555555 },
    { 0x66 },
    { 0x77, 0x88 },
    { Uint16{ 0x9999 } },
  };
  const bytes enc_struct =
    from_hex("111122222222333333334444444455555555016600027788BBBB9999");

  const std::optional<ExampleStruct> val_optional{ val_struct };
  const bytes enc_optional = from_hex("01") + enc_struct;

  const std::optional<ExampleStruct> val_optional_null = std::nullopt;
  const bytes enc_optional_null = from_hex("00");

  const IntSelector val_enum = IntSelector::uint8;
  const bytes enc_enum = from_hex("aaaa");
};

template<typename T>
void
ostream_test(T val, const std::vector<uint8_t>& enc)
{
  tls::ostream w;
  w << val;
  ASSERT_EQ(w.bytes(), enc);
}

TEST_F(TLSSyntaxTest, OStream)
{
  bytes answer{ 1, 2, 3, 4 };
  tls::ostream w;
  w.write_raw(answer);
  ASSERT_EQ(w.bytes(), answer);

  ostream_test(val_bool, enc_bool);
  ostream_test(val_uint8, enc_uint8);
  ostream_test(val_uint16, enc_uint16);
  ostream_test(val_uint32, enc_uint32);
  ostream_test(val_uint64, enc_uint64);
  ostream_test(val_array, enc_array);
  ostream_test(val_struct, enc_struct);
  ostream_test(val_optional, enc_optional);
  ostream_test(val_optional_null, enc_optional_null);
  ostream_test(val_enum, enc_enum);
}

template<typename T>
void
istream_test(T val, T& data, const std::vector<uint8_t>& enc)
{
  tls::istream r(enc);
  r >> data;
  ASSERT_EQ(data, val);
}

TEST_F(TLSSyntaxTest, IStream)
{
  bool data_bool;
  istream_test(val_bool, data_bool, enc_bool);

  uint8_t data_uint8;
  istream_test(val_uint8, data_uint8, enc_uint8);

  uint16_t data_uint16;
  istream_test(val_uint16, data_uint16, enc_uint16);

  uint32_t data_uint32;
  istream_test(val_uint32, data_uint32, enc_uint32);

  uint64_t data_uint64;
  istream_test(val_uint64, data_uint64, enc_uint64);

  std::array<uint16_t, 4> data_array;
  istream_test(val_array, data_array, enc_array);

  ExampleStruct data_struct;
  istream_test(val_struct, data_struct, enc_struct);

  std::optional<ExampleStruct> data_optional;
  istream_test(val_optional, data_optional, enc_optional);

  std::optional<ExampleStruct> data_optional_null;
  istream_test(val_optional_null, data_optional_null, enc_optional_null);

  IntSelector data_enum;
  istream_test(val_enum, data_enum, enc_enum);
}

TEST_F(TLSSyntaxTest, Abbreviations)
{
  ExampleStruct val_in = val_struct;

  tls::ostream w;
  w << val_struct;
  auto streamed = w.bytes();
  auto marshaled = tls::marshal(val_struct);
  ASSERT_EQ(streamed, marshaled);

  ExampleStruct val_out1{ 0 };
  tls::unmarshal(marshaled, val_out1);
  ASSERT_EQ(val_in, val_out1);

  auto val_out2 = tls::get<ExampleStruct>(marshaled);
  ASSERT_EQ(val_in, val_out2);
}

// TODO(rlb@ipv.sx) Test failure cases
