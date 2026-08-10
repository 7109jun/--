# AFDS Type System Specification v2.0 (FINAL)

> **개정 이력**
> - v1.0: 초기 45종 원시 타입 정의
> - v1.1: 자기비판 결과 반영 — `VARINT` 계열 정수, `Homogeneous Array`(Category `0x0E`) 추가로 벌크 전송 오버헤드 제거
> - v2.0 (FINAL): **Composite Type**(`STRUCT`, `MAP`, `UNION`, `OPTIONAL`, Category `0x0D`)과 이를 지원하는 **Schema Registry 메커니즘**을 확정하여 Type System 전체를 완결. 이후 기존 Type ID의 재정의는 금지되며 신규 Category 추가로만 확장한다 (§12)

## 1. 개요

AFDS의 Type System은 "데이터 타입을 최대한 명확하게 구분한다"는 원칙을 따른다.
모든 Type은:

1. 고유한 **Type ID** (2 bytes, `Category(1 byte) + Subtype(1 byte)`)를 가진다.
2. **정확한 binary encoding 규칙**을 가진다 (크기, 필드 순서, endianness, 특수값 처리).
3. binary representation이 명확히 정의될 수 없는 타입은 등록하지 않는다 (§8 참조).

---

## 2. Type ID 구조

```
Type ID = 0xCCSS
          CC = Category (1 byte)
          SS = Subtype  (1 byte)
```

- Category 0x00 ~ 0x0D: 본 스펙에서 정의
- Category 0x0E ~ 0xFE: 향후 확장 예약
- Category 0xFF: Vendor/실험적 타입 예약 (spec 밖)
- Subtype 0x00: 해당 Category 내 "정의되지 않음/Reserved"로 항상 예약

## 3. Endianness 정책

- 모든 다중 바이트 스칼라 필드는 기본적으로 **Little-Endian**으로 인코딩한다 (x86/ARM 양쪽 네이티브 표현과 일치, memcpy 호환성 우선).
- 예외: `UUID`는 RFC 4122 관례상 필드별 Big-Endian을 유지한다 (§4.9에서 별도 명시).
- 가변 길이 필드의 length prefix는 **LEB128 varint (unsigned)**를 사용한다.

---

## 4. 상세 Type 정의

### 4.1 Signed Integer — Category `0x02`

| Type ID | Name    | Size (bytes) | 표현 방식 | 범위 |
|---|---|---|---|---|
| 0x0201 | INT8   | 1  | 2's complement | -2^7 ~ 2^7-1 |
| 0x0202 | INT16  | 2  | 2's complement, LE | -2^15 ~ 2^15-1 |
| 0x0203 | INT32  | 4  | 2's complement, LE | -2^31 ~ 2^31-1 |
| 0x0204 | INT64  | 8  | 2's complement, LE | -2^63 ~ 2^63-1 |
| 0x0205 | INT128 | 16 | 2's complement, LE | -2^127 ~ 2^127-1 |
| 0x0206 | INT_VAR | 1~10 (가변) | **ZigZag + LEB128 varint**. `zigzag(n) = (n << 1) ^ (n >> 63)` 적용 후 unsigned varint 인코딩 | 값 범위상 INT64와 동일하나 절대값이 작을수록 1~2바이트로 저장됨 |

> **INT_VAR을 언제 쓰는가**: 값의 분포가 작은 경우가 많은 스트림(카운터, 델타값, 좌표 오프셋 등)에서 고정폭 INT32/INT64 대신 사용하면 전송량이 크게 줄어든다. 반대로 SIMD 처리, 고정 오프셋 랜덤 액세스가 필요한 배열에는 여전히 고정폭 정수(INT32 등)를 사용해야 한다 — 즉 둘은 대체재가 아니라 상호보완적 선택지다.

### 4.2 Unsigned Integer — Category `0x03`

| Type ID | Name    | Size | 표현 방식 |
|---|---|---|---|
| 0x0301 | UINT8   | 1  | 순수 이진, LE |
| 0x0302 | UINT16  | 2  | 순수 이진, LE |
| 0x0303 | UINT32  | 4  | 순수 이진, LE |
| 0x0304 | UINT64  | 8  | 순수 이진, LE |
| 0x0305 | UINT128 | 16 | 순수 이진, LE |
| 0x0306 | UINT_VAR | 1~10 (가변) | **LEB128 unsigned varint** (부호 없음, ZigZag 불필요) |

### 4.3 Floating Point (IEEE 754 계열) — Category `0x04`

모든 Float 타입은 `[Sign(1)][Exponent(E)][Mantissa(M)]` 순서의 bit layout을 가지며, IEEE 754의 규약(정규화, 비정규화, ±Inf, NaN, ±0)을 따른다.

| Type ID | Name | Size | Sign | Exponent(bit,bias) | Mantissa(bit) | 비고 |
|---|---|---|---|---|---|---|
| 0x0401 | FLOAT16  | 2 bytes  | 1 | 5, bias 15    | 10 (암묵적 leading 1) | IEEE 754 binary16 (half) |
| 0x0402 | FLOAT32  | 4 bytes  | 1 | 8, bias 127   | 23 (암묵적 leading 1) | IEEE 754 binary32 (single) |
| 0x0403 | FLOAT64  | 8 bytes  | 1 | 11, bias 1023 | 52 (암묵적 leading 1) | IEEE 754 binary64 (double) |
| 0x0404 | FLOAT80  | 10 bytes | 1 | 15, bias 16383| 64 (명시적 leading bit 포함, 암묵 없음) | x87 Extended Precision. **주의**: 메모리 정렬상 12/16바이트로 패딩되는 플랫폼이 있으나, AFDS 와이어 포맷은 항상 **패딩 없는 10바이트**로 고정한다 |
| 0x0405 | FLOAT128 | 16 bytes | 1 | 15, bias 16383| 112 (암묵적 leading 1) | IEEE 754 binary128 (quad) |
| 0x0406 | BFLOAT16 | 2 bytes  | 1 | 8, bias 127   | 7 (암묵적 leading 1)  | Google Brain Float, FLOAT32 상위 16bit 절단형. ML 상호운용을 위해 추가 |

> FLOAT80을 제외한 전 타입은 순수 IEEE 754 binary interchange format이며, 특수값(NaN payload, 무한대, subnormal) 처리는 IEEE 754-2008 표준을 그대로 따른다. FLOAT80은 IEEE 754 표준 타입이 아니지만(공식 표준 외 x87 확장) binary layout이 Intel 매뉴얼로 완전히 고정되어 있어 등록 가능하다.

### 4.4 Decimal / Arbitrary Precision — Category `0x05`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0501 | DECIMAL32  | 4 bytes  | IEEE 754-2008 Decimal32, **BID(Binary Integer Decimal)** 인코딩 고정 (DPD 대신 BID로 통일하여 구현 단순화) |
| 0x0502 | DECIMAL64  | 8 bytes  | IEEE 754-2008 Decimal64, BID |
| 0x0503 | DECIMAL128 | 16 bytes | IEEE 754-2008 Decimal128, BID |
| 0x0504 | FIXED_POINT | 가변 (파라미터화) | `[BaseType:1byte][Scale:int8][Value:BaseType bytes]`. BaseType은 0x02xx/0x03xx(정수 계열) 중 하나를 가리키는 subtype 1바이트. 실제값 = raw_integer × 10^(-Scale) |
| 0x0505 | BIG_INTEGER | 가변 | `[Length:varint][Bytes: 2's complement, LE, Length bytes]` |
| 0x0506 | BIG_DECIMAL | 가변 | `[Unscaled: BIG_INTEGER][Scale: INT32]`. 실제값 = unscaled × 10^(-scale) (Java BigDecimal과 동일 모델) |
| 0x0507 | RATIONAL | 가변 | `[Numerator: BIG_INTEGER][Denominator: BIG_INTEGER]` (Denominator는 항상 양수로 정규화) |

### 4.5 Complex Number — Category `0x06`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0601 | COMPLEX64  | 8 bytes  | `[Real: FLOAT32][Imag: FLOAT32]` |
| 0x0602 | COMPLEX128 | 16 bytes | `[Real: FLOAT64][Imag: FLOAT64]` |

### 4.6 String / Text — Category `0x07`

| Type ID | Name | Encoding |
|---|---|---|
| 0x0701 | STRING_UTF8  | `[Length:varint (byte 수 기준)][UTF-8 bytes]` |
| 0x0702 | STRING_ASCII | `[Length:varint][ASCII bytes, 각 바이트 0x00~0x7F]`. 검증 실패 시 디코더 에러 |
| 0x0703 | CHAR32       | 4 bytes, 단일 UTF-32 코드포인트 (LE) |

### 4.7 Binary Data — Category `0x08`

| Type ID | Name | Encoding |
|---|---|---|
| 0x0801 | BYTES        | `[Length:varint][raw bytes]` |
| 0x0802 | FIXED_BYTES  | `[N:varint (메타데이터, 스키마 정의시 고정)][raw bytes, N bytes]` — 길이가 스키마 레벨에서 고정되는 경우용 |

### 4.8 Date / Time — Category `0x09`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0901 | DATE         | 4 bytes  | INT32, 1970-01-01 기준 signed 일(day) 수 |
| 0x0902 | TIME         | 8 bytes  | INT64, 자정 기준 signed 나노초 (0 ~ 86,399,999,999,999) |
| 0x0903 | DATETIME     | 8 bytes  | INT64, Unix epoch(1970-01-01T00:00:00Z) 기준 signed 나노초 |
| 0x0904 | DATETIME_TZ  | 10 bytes | `[DATETIME:INT64][OffsetMinutes:INT16]` (UTC 오프셋, -1440~1440) |
| 0x0905 | DURATION     | 8 bytes  | INT64, signed 나노초 |

### 4.9 Identifier — Category `0x0A`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0A01 | UUID | 16 bytes | RFC 4122 준수. 필드별 Big-Endian: `time_low(4)-time_mid(2)-time_hi_and_version(2)-clock_seq(2)-node(6)`, 전체가 표준 텍스트 표기와 바이트 순서상 1:1 대응 |

### 4.10 Network Address — Category `0x0B`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0B01 | IPV4        | 4 bytes  | Network byte order (Big-Endian), RFC 791 |
| 0x0B02 | IPV6        | 16 bytes | Network byte order (Big-Endian), RFC 8200 |
| 0x0B03 | MAC_ADDRESS | 6 bytes  | IEEE 802 표준 순서 (그대로 6바이트 나열) |

### 4.11 Color — Category `0x0C`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0C01 | COLOR_RGB8    | 3 bytes  | `[R:UINT8][G:UINT8][B:UINT8]` |
| 0x0C02 | COLOR_RGBA8   | 4 bytes  | `[R][G][B][A]`, 각 UINT8 |
| 0x0C03 | COLOR_RGB_F32 | 12 bytes | `[R][G][B]`, 각 FLOAT32 (HDR/linear 색공간용, 0.0~1.0 초과 허용) |
| 0x0C04 | COLOR_RGBA_F32| 16 bytes | `[R][G][B][A]`, 각 FLOAT32 |

### 4.12 기타 기본형 — Category `0x00`, `0x01`

| Type ID | Name | Size | Encoding |
|---|---|---|---|
| 0x0000 | NULL | 0 bytes | 값 없음, 타입 태그만 존재 |
| 0x0100 | BOOL | 1 byte | `0x00`=false, `0x01`=true, 그 외 값은 디코더 에러 (관대한 truthy 해석 금지) |

### 4.13 Composite Type — Category `0x0D`

| Type ID | Name | Encoding | 설명 |
|---|---|---|---|
| 0x0D01 | STRUCT_SELFDESC | `[FieldCount:varint]`, 이후 FieldCount회 반복: `[NameLen:varint][NameUTF8][FieldTypeID:2][FieldValue]` | 완전 자기서술형 구조체. 필드 이름·타입을 값과 함께 매번 전송. 스키마 사전 공유 불필요, 유연하나 오버헤드 큼 (디버깅/임시 데이터/스키마리스 상황용) |
| 0x0D02 | STRUCT_SCHEMA | `[SchemaID:UINT32][FieldValues: 스키마에 정의된 필드 순서대로, 태그 없이 값만 연속]` | 스키마 참조형 구조체. §11 Schema Registry의 SchemaID로 필드 이름/타입/순서를 참조하여 전송량 최소화. 프로덕션 벌크 전송에 권장 |
| 0x0D03 | MAP_HOMOGENEOUS | `[Count:varint][KeyTypeID:2][ValueTypeID:2][Count × (Key,Value), 원소별 태그 없음]` | 키/값 타입이 고정된 맵 (예: `Map<STRING_UTF8, INT32>`). 가변길이 Key/Value는 §4.14 ARRAY_VAR와 동일한 오프셋 테이블 규칙을 각각 별도 적용 |
| 0x0D04 | MAP_MIXED | `[Count:varint]`, 이후 Count회 반복: `[KeyTypeID:2][Key][ValueTypeID:2][Value]` | 엔트리마다 키/값 타입이 달라질 수 있는 이종 맵 (JSON object 상당). 유연하나 오버헤드 큼 |
| 0x0D05 | UNION | `[SelectedTypeID:2][Value: SelectedTypeID 규칙대로 인코딩]` | Tagged union/sum type. "이 필드는 INT32 또는 STRING_UTF8일 수 있다" 같은 다형 필드. 와이어 형태는 self-describing 스칼라와 동일하나, 스키마 상 "여러 타입 중 하나"라는 제약을 명시하는 의미론적 타입 |
| 0x0D06 | OPTIONAL | `[Presence:UINT8(0x00=없음,0x01=있음)][Value: Presence=0x01일 때만 존재]` | 단일 nullable 필드. 배열 단위 null은 §4.14 ARRAY_NULLABLE, 단일 필드 null은 본 타입 사용 |

> **재귀·중첩 규칙**: `STRUCT_*`/`MAP_*`/`UNION`/`OPTIONAL`의 Value 자리에는 Composite Type을 포함한 임의의 등록 Type ID가 올 수 있다(재귀 허용) — 예: STRUCT 안에 ARRAY_FIXED, 그 안에 다시 STRUCT_SCHEMA. 단 디코더는 **스택 오버플로/DoS 방지를 위해 최대 중첩 깊이(권장 기본값 64)를 강제**해야 하며, 초과 시 명시적 에러로 거부해야 한다.

### 4.14 Homogeneous Array (Bulk Encoding) — Category `0x0E`

**§9의 자기비판에서 지적된 "값마다 Type ID 2바이트 반복" 문제를 해결하기 위한 타입군.** 동일 타입 원소가 대량으로 나열되는 경우(배열, 컬럼, 스트림) 원소 하나하나에 태그를 붙이지 않고, **Type ID를 컨테이너 전체에 대해 단 한 번만** 기록한다.

| Type ID | Name | Encoding | 적용 대상 |
|---|---|---|---|
| 0x0E01 | ARRAY_FIXED | `[ElementTypeID:2][Count:varint][RawPayload: Count × ElementSize bytes, 원소별 태그 없음]` | 원소 크기가 고정인 모든 타입 (INT*, UINT*, FLOAT*, COMPLEX*, UUID, IPV4/6, COLOR* 등) |
| 0x0E02 | ARRAY_VAR | `[ElementTypeID:2][Count:varint][OffsetTable: (Count+1) × UINT32][DataBlock: 원소 payload를 이어붙인 것]` | 원소 크기가 가변인 타입 (STRING_UTF8, BYTES, BIG_INTEGER 등). OffsetTable[i]~OffsetTable[i+1] 구간이 i번째 원소 |
| 0x0E03 | ARRAY_NULLABLE | `[ElementTypeID:2][Count:varint][ValidityBitmap: ceil(Count/8) bytes][RawPayload or (ARRAY_FIXED/ARRAY_VAR 규칙에 준함), null 슬롯도 자리 유지]` | null 허용이 필요한 배열. Apache Arrow의 validity bitmap과 동일한 모델 |

**오버헤드 비교 (INT32 원소 1,000개 기준)**
- 기존(값마다 TypeID) 방식: `1000 × (2 + 4) = 6,000 bytes`, 순수 오버헤드 2,000 bytes (33%)
- ARRAY_FIXED 방식: `2(ElementTypeID) + ~2(Count varint) + 1000×4 = 4,004 bytes`, 오버헤드 0.1% 미만

이로써 AFDS는 "필드 하나하나엔 self-describing 태깅(디버깅·유연성 우선)"과 "배열·스트림 구간엔 태그리스 벌크 인코딩(전송 효율 우선)"을 **명시적으로 구분해서 선택 가능**하게 된다. 어느 쪽도 강제하지 않고, 데이터 성격에 맞게 인코더가 고를 수 있는 구조다.

---

## 5. Type ID Registry 요약표

| Category | 이름 | Subtype 개수 |
|---|---|---|
| 0x00 | NULL | 1 |
| 0x01 | BOOLEAN | 1 |
| 0x02 | SIGNED INTEGER | 6 (INT_VAR 포함) |
| 0x03 | UNSIGNED INTEGER | 6 (UINT_VAR 포함) |
| 0x04 | FLOATING POINT | 6 |
| 0x05 | DECIMAL/ARBITRARY PRECISION | 7 |
| 0x06 | COMPLEX | 2 |
| 0x07 | STRING/TEXT | 3 |
| 0x08 | BINARY | 2 |
| 0x09 | DATE/TIME | 5 |
| 0x0A | IDENTIFIER (UUID) | 1 |
| 0x0B | NETWORK ADDRESS | 3 |
| 0x0C | COLOR | 4 |
| 0x0D | MAP/STRUCT (예약, Composite 스펙 이관) | - |
| 0x0E | HOMOGENEOUS ARRAY (Bulk) | 3 |

총 49개 원시/벌크 Type ID 정의.

---

## 6. 파라미터화 타입 (Parameterized Type) 인코딩 규칙

`FIXED_POINT`, `FIXED_BYTES`처럼 고정 크기가 아닌 타입은, Type ID 뒤에 **메타데이터 바이트열**이 스키마 정의 시점에 함께 기록된다. 즉 와이어 상에서 "타입" 자체가 `[TypeID(2)][Metadata(가변)]`로 구성되며, 실제 값은 그 뒤에 이어진다. 이는 스트림 레벨(값마다 매번 타입 반복)과 스키마 레벨(한 번만 정의 후 값들만 반복) 양쪽에서 동일하게 적용 가능하다.

---

## 7. 검토했으나 제외한 타입과 사유

| 검토 대상 | 제외 사유 |
|---|---|
| MONEY / CURRENCY | 통화 코드, 반올림 규칙이 도메인/로케일 종속적이라 "하나의 정확한 바이너리 표현"으로 고정 불가. `BIG_DECIMAL` + 애플리케이션 레벨 통화 필드 조합을 권장 |
| TRIT / TERNARY | 표준화된 비트 패킹 방식이 없음 (2bit/3trit 패킹 등 구현체마다 상이) |
| STRING_UTF16 | UTF-8 대비 실질적 이점이 제한적이고 BOM/서로게이트 페어 처리로 모호성이 늘어나 1.0에서는 제외. 필요 시 `0x0704`로 추후 추가 가능하도록 예약만 해둠 |
| POSIT (Type III Unum) | 비트 레이아웃 표준(exponent size 등)이 프로젝트마다 다르게 구현되어 "고정된" 정의가 아직 산업 표준화되지 않음 |
| IPV4_CIDR / IPV6_CIDR | 프리픽스 길이까지 포함하면 명확히 정의 가능하나, 이는 "주소" 자체가 아니라 "주소+라우팅 정보" 복합 개념이므로 Composite Type 스펙으로 이관 |

---

## 8. 설계 원칙 요약

1. **명확성 우선**: 모든 Type은 크기·필드 순서·엔디안이 모호함 없이 1가지로만 해석되어야 한다.
2. **IEEE 754 정합성**: 부동소수점은 임의로 이름만 늘리지 않고, 실제 표준(또는 산업적으로 고정된 규약)이 있는 것만 등록한다.
3. **확장 가능성**: Category/Subtype 체계로 향후 신규 타입 추가 시 기존 ID와 충돌 없이 확장 가능.
4. **가변 길이 타입의 명시적 규칙화**: "바이너리로 표현 불가능해 보이는" BIG_INTEGER, RATIONAL 등도 length-prefix + 정렬 규칙을 부여해 명확히 등록했다.

---

## 9. 자기비판 (Self-Critique) 및 v1.1 개선 근거

v1.0을 "실제로 네트워크로 데이터를 보낸다"는 관점에서 재검토한 결과, 다음 4가지 구조적 문제를 발견했다.

### 9.1 (치명적) 값마다 Type ID를 반복 태깅함 → 벌크 전송에 매우 비효율적

v1.0의 모든 값은 `[TypeID:2][Value]` 구조였다. 이는 JSON이 모든 필드마다 키 문자열을 반복해서 전송하는 것과 본질적으로 같은 문제다 — **원소 하나하나가 자기 타입을 스스로 설명해야 하는 self-describing 방식은, 같은 타입 값이 대량으로 반복될 때 압도적으로 비효율적**이다.

- 예: INT32 100만 개 배열 → 기존 방식 6,000,000 bytes 중 2,000,000 bytes(33%)가 순수 오버헤드
- **개선**: §4.14에서 `ARRAY_FIXED / ARRAY_VAR / ARRAY_NULLABLE` (Category `0x0E`)을 신설해, 동일 타입 반복 구간은 TypeID를 컨테이너당 1회만 기록하도록 구조를 분리했다.

### 9.2 고정폭 정수만 존재 → 작은 값이 많은 실데이터에서 낭비

실제 트래픽 데이터(카운터, 타임스탬프 델타, 좌표 오프셋 등)는 절대값이 작은 경우가 압도적으로 많은데, v1.0은 INT32/INT64 등 고정폭만 강제해 항상 4~8바이트를 소모했다.

- **개선**: §4.1/4.2에 `INT_VAR`(ZigZag+LEB128), `UINT_VAR`(LEB128)을 추가. 단, 고정 오프셋 랜덤 액세스나 SIMD 처리가 필요한 경우엔 여전히 고정폭 타입을 권장하도록 병기했다 — VARINT가 고정폭을 대체하는 게 아니라 선택지를 넓히는 것임을 명확히 했다.

### 9.3 (구조적 한계, 완전 해결은 아님) Endianness 혼용

스칼라는 Little-Endian, `UUID`/`IPV4`/`IPV6`는 Big-Endian(Network byte order)을 쓴다. 이는 디코더가 타입에 따라 분기해야 하므로 핫패스 성능에 불리하다.

- **판단**: 이 혼용은 실수가 아니라 **기존 프로토콜과의 상호운용성**(IPv4/IPv6은 이미 전 세계 네트워크 스택이 Big-Endian을 전제로 함, UUID도 RFC 4122 텍스트 표현과 1:1 대응되어야 함)을 위한 의도적 트레이드오프다. 다만 v1.0에는 이 판단 근거가 명시되어 있지 않아 "일관성 없음"으로 오해될 수 있었으므로, §3에 판단 근거를 보강했다. 완전한 해결책(예: 항상 LE로 통일하고 소켓 전송 계층에서만 변환)은 상호운용성을 해치므로 채택하지 않는다.

### 9.4 (미해결, 후속 문서 과제) 압축과의 상호작용 미검토

VARINT/ARRAY_FIXED 도입만으로는 반복 패턴이 많은 데이터(예: 같은 값이 계속 반복되는 센서 로그)의 엔트로피 중복은 제거되지 않는다. 이는 Type System이 아니라 별도의 **Transport/Compression Layer**(예: 필드별 dictionary encoding, RLE)에서 다뤄야 할 문제이므로, 이번 개정 범위에서는 명시적으로 범위 밖으로 남겨두고 후속 스펙 과제로 기록한다.
