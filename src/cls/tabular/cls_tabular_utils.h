/*
* Copyright (C) 2018 The Regents of the University of California
* All Rights Reserved
*
* This library can redistribute it and/or modify under the terms
* of the GNU Lesser General Public License Version 2.1 as published
* by the Free Software Foundation.
*
*/


#ifndef CLS_TABULAR_UTILS_H
#define CLS_TABULAR_UTILS_H

#include <string>
#include <sstream>
#include <type_traits>

#include "include/types.h"
#include <errno.h>
#include <time.h>
#include <boost/algorithm/string/classification.hpp> // for boost::is_any_of
#include <boost/algorithm/string/split.hpp> // for boost::split
#include <boost/algorithm/string.hpp> // for boost::trim
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>

#include "re2/re2.h"
#include "objclass/objclass.h"

#include "cls_tabular.h"
#include "flatbuffers/flexbuffers.h"
#include "skyhookv2_generated.h"

namespace Tables {

enum TablesErrCodes {
    EmptySchema = 1, // note: must start at 1
    BadColInfoFormat,
    BadColInfoConversion,
    UnsupportedSkyDataType,
    UnsupportedAggDataType,
    UnknownSkyDataType,
    RequestedColIndexOOB,
    RequestedColNotPresent,
    PredicateComparisonNotDefined,
    PredicateRegexPatternNotSet,
    OpNotRecognized,
    OpNotImplemented,
    BuildSkyIndexDecodeBlsErr,
    BuildSkyIndexExtractFbErr,
    BuildSkyIndexUnsupportedColType,
    BuildSkyIndexColTypeNotImplemented,
    BuildSkyIndexUnsupportedNumCols,
    BuildSkyIndexUnsupportedAggCol,
    BuildSkyIndexKeyCreationFailed,
    BuildSkyIndexColIndexOOB,
    SkyIndexUnsupportedOpType,
    SkyIndexColNotPresent,
    RowIndexOOB,
    SkyFormatTypeNotImplemented
};

// skyhook data types, as supported by underlying data format
enum SkyDataType {
    SDT_INT8 = 1,  // note: must start at 1
    SDT_INT16,
    SDT_INT32,
    SDT_INT64,
    SDT_UINT8,
    SDT_UINT16,
    SDT_UINT32,
    SDT_UINT64,
    SDT_CHAR,
    SDT_UCHAR,
    SDT_BOOL,
    SDT_FLOAT,
    SDT_DOUBLE,
    SDT_DATE,
    SDT_STRING,
    /*
    SDT_BLOB,  // TODO
    SDT_VECTOR_INT8,  // TODO
    SDT_VECTOR_INT64,  // TODO
    SDT_VECTOR_UINT8,  // TODO
    SDT_VECTOR_UINT64,  // TODO
    SDT_VECTOR_DOUBLE,  // TODO
    SDT_VECTOR_STRING,  // TODO
    */
    SDT_FIRST = SDT_INT8,
    SDT_LAST = SDT_STRING,
};

enum SkyOpType {
    // ARITHMETIC COMPARISON
    SOT_lt = 1,  // note: must start at 1
    SOT_gt,
    SOT_eq,
    SOT_ne,
    SOT_leq,
    SOT_geq,
    // ARITHMETIC FUNCTIONS (TODO)
    SOT_add,
    SOT_sub,
    SOT_mul,
    SOT_div,
    SOT_min,
    SOT_max,
    SOT_sum,
    SOT_cnt,
    // LEXICAL (regex)
    SOT_like,
    // MEMBERSHIP (collections) (TODO)
    SOT_in,
    SOT_not_in,
    // RANGE (TODO)
    SOT_between,
    // DATE (SQL)
    SOT_before,
    SOT_after,
    // LOGICAL
    SOT_logical_or,
    SOT_logical_and,
    SOT_logical_not,
    SOT_logical_nor,
    SOT_logical_xor,
    SOT_logical_nand,
    // BITWISE
    SOT_bitwise_and,
    SOT_bitwise_or,
    SOT_FIRST = SOT_lt,
    SOT_LAST = SOT_bitwise_or,
};

enum SkyIdxType
{
    SIT_IDX_FB,
    SIT_IDX_RID,
    SIT_IDX_REC,
    SIT_IDX_TXT,
    SIT_IDX_UNK
};

enum IdxField {
    IDX_FIELD_TYPE = 0,
    IDX_FIELD_Table,
    IDX_FIELD_Column,
    IDX_FIELD_Value,
};

enum SkyIdxPlanType
{
    SIP_IDX_STANDARD = 1,
    SIP_IDX_INTERSECTION,
    SIP_IDX_UNION
};

const std::map<SkyIdxType, std::string> SkyIdxTypeMap = {
    {SIT_IDX_FB, "IDX_FBF"},
    {SIT_IDX_RID, "IDX_RID"},
    {SIT_IDX_REC, "IDX_REC"},
    {SIT_IDX_TXT, "IDX_TXT"},
    {SIT_IDX_UNK, "IDX_UNK"}
};

// sampling density of statistics collected
enum StatsLevel {
    LOW=1,
    MED,
    HIGH,
};

enum AggColIdx {
    AGG_COL_MIN = -1,  // defines the schema idx for agg ops.
    AGG_COL_MAX = -2,
    AGG_COL_SUM = -3,
    AGG_COL_CNT = -4,
    AGG_COL_FIRST = AGG_COL_MIN,
    AGG_COL_LAST = AGG_COL_CNT,
};

const std::map<std::string, int> AGG_COL_IDX = {
    {"min", AGG_COL_MIN},
    {"max", AGG_COL_MAX},
    {"sum", AGG_COL_SUM},
    {"cnt", AGG_COL_CNT}
};

const std::unordered_map<std::string, bool> IDX_STOPWORDS= {
    {"a", 1}, {"an", 1}, {"and", 1}, {"are",  1}, {"as", 1}, {"at", 1},
    {"be", 1}, {"by", 1}, {"for", 1}, {"from", 1},
    {"has", 1}, {"he", 1},
    {"in", 1}, {"is", 1}, {"it", 1}, {"its", 1},
    {"of", 1}, {"on", 1},
    {"that", 1}, {"the", 1}, {"to", 1},
    {"was", 1}, {"were", 1}, {"will", 1}, {"with", 1}
};

const int offset_to_skyhook_version = 4;
const int offset_to_schema_version = 6;
const int offset_to_table_name = 8;
const int offset_to_schema = 10;
const int offset_to_delete_vec = 12;
const int offset_to_rows_vec = 14;
const int offset_to_nrows = 16;
const int offset_to_RID = 4;
const int offset_to_nullbits_vec = 6;
const int offset_to_data = 8;
const std::string PRED_DELIM_OUTER = ";";
const std::string PRED_DELIM_INNER = ",";
const std::string PROJECT_DEFAULT = "*";
const std::string SELECT_DEFAULT = "*";
const std::string REGEX_DEFAULT_PATTERN = "/.^/";  // matches nothing.
const int MAX_COLSIZE = 4096; // primarily for text cols TODO: blobs
const int MAX_TABLE_COLS = 128; // affects nullbits vector size (skyroot)
const int MAX_INDEX_COLS = 4;
const int DATASTRUCT_SEQ_NUM_MIN = 0;
const int DATASTRUCT_SEQ_NUM_MAX = 10000;  // max per obj, before compaction
const char CSV_DELIM = '|';
const std::string IDX_KEY_DELIM_INNER = "-";
const std::string IDX_KEY_DELIM_OUTER = ":";
const std::string IDX_KEY_DELIM_UNIQUE = "ENFORCEUNIQ";
const std::string IDX_KEY_COLS_DEFAULT = "*";
const std::string SCHEMA_NAME_DEFAULT = "*";
const std::string TABLE_NAME_DEFAULT = "*";
const std::string RID_INDEX = "_RID_INDEX_";
const int RID_COL_INDEX = -99; // magic number...
const long long int ROW_LIMIT_DEFAULT = LLONG_MAX;

/*
 * Convert integer to string for index/omap of primary key
 */
static inline std::string u64tostr(uint64_t value)
{
  std::stringstream ss;
  ss << std::setw(20) << std::setfill('0') << value;
  return ss.str();
}

/*
 * Convert string into numeric value.
 */
static inline int strtou64(const std::string value, uint64_t *out)
{
  uint64_t v;

  try {
    v = boost::lexical_cast<uint64_t>(value);
  } catch (boost::bad_lexical_cast &) {
    CLS_ERR("converting key into numeric value %s", value.c_str());
    return -EIO;
  }

  *out = v;
  return 0;
}

// contains the value of a predicate to be applied
template <class T>
class PredicateValue
{
public:
    T val;
    PredicateValue(T v) : val(v) {};
    PredicateValue(const PredicateValue& rhs);
    PredicateValue& operator=(const PredicateValue& rhs);
};

// PredBase is not template typed, derived is type templated,
// allows us to have vectors of chained base class predicates
class PredicateBase
{
public:
    virtual ~PredicateBase() {}
    virtual int colIdx() = 0;  // to check info via base ptr before dynm cast
    virtual int colType() = 0;
    virtual int opType() = 0;
    virtual int chainOpType() = 0;
    virtual bool isGlobalAgg() = 0;
};
typedef std::vector<class PredicateBase*> predicate_vec;

template <typename T>
class TypedPredicate : public PredicateBase
{
private:
    const int col_idx;
    const int col_type;
    const int op_type;
    const bool is_global_agg;
    const re2::RE2* regx;
    PredicateValue<T> value;
    const int chain_op_type;

public:
    TypedPredicate(int idx, int type, int op, const T& val, const int ch_op=SOT_logical_and) :
        col_idx(idx),
        col_type(type),
        op_type(op),
        is_global_agg(op==SOT_min || op==SOT_max ||
                      op==SOT_sum || op==SOT_cnt),
        value(val),
        chain_op_type(ch_op) {

            // ONLY VERIFY op type is valid for specified col type and value
            // type T, and compile regex if needed.
            switch (op_type) {
                // ARITHMETIC/COMPARISON
                case SOT_lt:
                case SOT_gt:
                case SOT_eq:
                case SOT_ne:
                case SOT_leq:
                case SOT_geq:
                case SOT_add:
                case SOT_sub:
                case SOT_mul:
                case SOT_div:
                case SOT_min:
                case SOT_max:
                case SOT_sum:
                case SOT_cnt:
                    assert (
                            (col_type==SDT_DATE) or
                            (std::is_arithmetic<T>::value and
                            (col_type==SDT_INT8 ||
                             col_type==SDT_INT16 ||
                             col_type==SDT_INT32 ||
                             col_type==SDT_INT64 ||
                             col_type==SDT_UINT8 ||
                             col_type==SDT_UINT16 ||
                             col_type==SDT_UINT32 ||
                             col_type==SDT_UINT64 ||
                             col_type==SDT_BOOL ||
                             col_type==SDT_CHAR ||
                             col_type==SDT_UCHAR ||
                             col_type==SDT_FLOAT ||
                             col_type==SDT_DOUBLE))
                            );
                    break;

                // LEXICAL (regex)
                case SOT_like:
                    assert (
                            ((std::is_same<T, std::string>::value) == true ||
                             (std::is_same<T, unsigned char>::value) == true ||
                             (std::is_same<T, char>::value) == true)
                            and
                            (col_type==SDT_STRING ||
                             col_type==SDT_CHAR ||
                             col_type==SDT_UCHAR)
                            );
                    break;

                // MEMBERSHIP (collections)
                case SOT_in:
                case SOT_not_in:
                    assert(((std::is_same<T, std::vector<T>>::value) == true));
                    assert (TablesErrCodes::OpNotImplemented==0); // TODO
                    break;

                // DATE (SQL)
                case SOT_before:
                case SOT_between:
                case SOT_after:
                    assert (col_type==SDT_DATE);  // TODO
                    break;

                // LOGICAL
                case SOT_logical_or:
                case SOT_logical_and:
                case SOT_logical_not:
                case SOT_logical_nor:
                case SOT_logical_xor:
                case SOT_logical_nand:
                    assert (std::is_integral<T>::value);  // includes bool+char
                    assert (col_type==SDT_INT8 ||
                            col_type==SDT_INT16 ||
                            col_type==SDT_INT32 ||
                            col_type==SDT_INT64 ||
                            col_type==SDT_UINT8 ||
                            col_type==SDT_UINT16 ||
                            col_type==SDT_UINT32 ||
                            col_type==SDT_UINT64 ||
                            col_type==SDT_BOOL ||
                            col_type==SDT_CHAR ||
                            col_type==SDT_UCHAR);
                    break;

                // BITWISE
                case SOT_bitwise_and:
                case SOT_bitwise_or:
                    assert (std::is_integral<T>::value);
                    assert (std::is_unsigned<T>::value);
                    break;

                // FALL THROUGH OP not recognized
                default:
                    assert (TablesErrCodes::OpNotRecognized==0);
                    break;
            }

            // compile regex
            std::string pattern;
            if (op_type == SOT_like) {
                pattern = this->Val();  // force str type for regex
                regx = new re2::RE2(pattern);
                assert (regx->ok());
            }
        }

    TypedPredicate(const TypedPredicate &p) : // copy constructor as needed
        col_idx(p.col_idx),
        col_type(p.col_type),
        op_type(p.op_type),
        is_global_agg(p.is_global_agg),
        value(p.value.val) {
            regx = new re2::RE2(p.regx->pattern());
        }

    ~TypedPredicate() { }
    TypedPredicate& getThis() {return *this;}
    const TypedPredicate& getThis() const {return *this;}
    virtual int colIdx() {return col_idx;}
    virtual int colType() {return col_type;}
    virtual int opType() {return op_type;}
    virtual int chainOpType() {return chain_op_type;}
    virtual bool isGlobalAgg() {return is_global_agg;}
    T Val() {return value.val;}
    const re2::RE2* getRegex() {return regx;}
    void updateAgg(T newval) {value.val = newval;}
};

// col metadata used for the schema
const int NUM_COL_INFO_FIELDS = 5;
struct col_info {
    const int idx;
    const int type;
    const bool is_key;
    const bool nullable;
    const std::string name;

    col_info(int i, int t, bool key, bool nulls, std::string n) :
        idx(i),
        type(t),
        is_key(key),
        nullable(nulls),
        name(n) {
            assert(type >= SDT_FIRST && type <= SDT_LAST);
        }

    col_info(std::string i, std::string t, std::string key, std::string nulls,
             std::string n) :
        idx(std::stoi(i.c_str())),
        type(std::stoi(t.c_str())),
        is_key(key[0]=='1'),
        nullable(nulls[0]=='1'),
        name(n) {
            assert(type >= SDT_FIRST && type <= SDT_LAST);
        }

    col_info(std::vector<std::string> s) : // unsafe, for testing only
        idx(stoll(s[0])),
        type(stoll(s[1])),
        is_key(s[2]=="1"),
        nullable(s[3]=="1"),
        name(s[4]) {
            assert (s.size() == NUM_COL_INFO_FIELDS);
        }

    col_info(const col_info& c) :
        idx(c.idx),
        type(c.type),
        is_key(c.is_key),
        nullable(c.nullable),
        name(c.name) {}

    std::string toString() {
        return ( "   " +
            std::to_string(idx) + " " +
            std::to_string(type) + " " +
            std::to_string(is_key) + " " +
            std::to_string(nullable) + " " +
            name + "   ");}

    inline bool compareName(std::string colname) {
        return (colname==name) ? true : false;
    }
};
typedef vector<struct col_info> schema_vec;

inline
bool compareColInfo(const struct col_info& l, const struct col_info& r) {
    return (
        (l.idx==r.idx) &&
        (l.type==r.type) &&
        (l.is_key==r.is_key) &&
        (l.nullable==r.nullable) &&
        (l.name==r.name)
    );
}

// the below are used in our root table
typedef vector<uint8_t> delete_vector;
typedef const flatbuffers::Vector<flatbuffers::Offset<Record>>* row_offs;

// the below are used in our row table
typedef vector<uint64_t> nullbits_vector;
typedef flexbuffers::Reference row_data_ref;

// skyhookdb root metadata, refering to a (sub)partition of rows
// abstracts a partition from its underlying data format/layout
struct root_table {

    int32_t skyhook_version;
    int32_t data_format_type;
    int32_t data_structure_version;
    int32_t data_schema_version;
    std::string data_schema;
    std::string db_schema;
    std::string table_name;
    delete_vector delete_vec;
    row_offs offs;
    uint32_t nrows;

    root_table(
        int32_t _data_format_type,
        int32_t _skyhook_version,
	int32_t _data_structure_version,
	int32_t _data_schema_version,
        std::string _data_schema,
        std::string _db_schema,
        std::string _table_name,
        delete_vector _delete_vec,
        row_offs _offs,
        uint32_t _nrows) :  data_format_type(_data_format_type),
	    		    skyhook_version(_skyhook_version),
                            data_structure_version(_data_structure_version),
			    data_schema_version(_data_schema_version),
                            data_schema(_data_schema),
                            db_schema(_db_schema),
                            table_name(_table_name),
                            delete_vec(_delete_vec),
                            offs(_offs),
                            nrows(_nrows) {};
};
typedef struct root_table sky_root;

// skyhookdb row metadata and row data, wraps a row of data
// abstracts a row from its underlying data format/layout
struct rec_table {
    const int64_t RID;
    nullbits_vector nullbits;
    const row_data_ref data;

    rec_table(int64_t _RID, nullbits_vector _nullbits, row_data_ref _data) :
        RID(_RID),
        nullbits(_nullbits),
        data(_data) {
            // ensure one nullbit per col
            int num_nullbits = nullbits.size() * sizeof(nullbits[0]) * 8;
            assert (num_nullbits == MAX_TABLE_COLS);
        };
};
typedef struct rec_table sky_rec;

// holds the result of a read to be done, resulting from an index lookup
// regarding specific flatbufs+rows to be read or else a seq of all flatbufs
// for which this struct is used to identify the physical location of the
// flatbuf and its specified row numbers (default=all) to be processed.
struct read_info {
    int fb_seq_num;
    int off;
    int len;
    std::vector<unsigned int> rnums;  //default to empty to read all rows

    read_info(int _fb_seq_num,
              int _off,
              int _len,
              std::vector<unsigned> _rnums) :
        fb_seq_num(_fb_seq_num),
        off(_off),
        len(_len),
        rnums(_rnums) {};

    read_info(const read_info& r) :
        fb_seq_num(r.fb_seq_num),
        off(r.off),
        len(r.len),
        rnums(r.rnums) {};

    read_info() :
        fb_seq_num(),
        off(),
        len(),
        rnums() {};

    std::string toString() {
        std::string rows_str;
        for (auto it = rnums.begin(); it != rnums.end(); ++it)
            rows_str.append((std::to_string(*it) + ","));
        std::string s;
        s.append("index_read_info.fb_num=" + std::to_string(fb_seq_num));
        s.append("; index_read_info.off=" + std::to_string(off));
        s.append("; index_read_info.len=" + std::to_string(len));
        s.append("; index_read_info.rnums=" + rows_str);
        return s;
    }
};

const std::string SCHEMA_FORMAT ( \
        "\ncol_idx col_type is_key is_nullable name \\n" \
        "\ncol_idx col_type is_key is_nullable name \\n" \
        "\n ... \\n");

// A test schema for the TPC-H lineitem table.
// format: "col_idx col_type is_key is_nullable name"
// note the col_idx always refers to the index in the table's current schema
const std::string TPCH_LINEITEM_TEST_SCHEMA_STRING (" \
    0 " +  std::to_string(SDT_INT32) + " 1 0 ORDERKEY \n\
    1 " +  std::to_string(SDT_INT32) + " 0 1 PARTKEY \n\
    2 " +  std::to_string(SDT_INT32) + " 0 1 SUPPKEY \n\
    3 " +  std::to_string(SDT_INT32) + " 1 0 LINENUMBER \n\
    4 " +  std::to_string(SDT_FLOAT) + " 0 1 QUANTITY \n\
    5 " +  std::to_string(SDT_DOUBLE) + " 0 1 EXTENDEDPRICE \n\
    6 " +  std::to_string(SDT_FLOAT) + " 0 1 DISCOUNT \n\
    7 " +  std::to_string(SDT_DOUBLE) + " 0 1 TAX \n\
    8 " +  std::to_string(SDT_CHAR) + " 0 1 RETURNFLAG \n\
    9 " +  std::to_string(SDT_CHAR) + " 0 1 LINESTATUS \n\
    10 " +  std::to_string(SDT_DATE) + " 0 1 SHIPDATE \n\
    11 " +  std::to_string(SDT_DATE) + " 0 1 COMMITDATE \n\
    12 " +  std::to_string(SDT_DATE) + " 0 1 RECEIPTDATE \n\
    13 " +  std::to_string(SDT_STRING) + " 0 1 SHIPINSTRUCT \n\
    14 " +  std::to_string(SDT_STRING) + " 0 1 SHIPMODE \n\
    15 " +  std::to_string(SDT_STRING) + " 0 1 COMMENT \n\
    ");

// A test schema for projection over the TPC-H lineitem table.
const std::string TPCH_LINEITEM_TEST_SCHEMA_STRING_PROJECT = " \
    0 " +  std::to_string(SDT_INT32) + " 1 0 ORDERKEY \n\
    1 " +  std::to_string(SDT_INT32) + " 0 1 PARTKEY \n\
    3 " +  std::to_string(SDT_INT32) + " 1 0 LINENUMBER \n\
    4 " +  std::to_string(SDT_FLOAT) + " 0 1 QUANTITY \n\
    5 " +  std::to_string(SDT_DOUBLE) + " 0 1 EXTENDEDPRICE \n\
    ";

// these extract the current data format (flatbuf) into the skyhookdb
// root table and row table data structure defined above, abstracting
// skyhookdb data partitions design from the underlying data format.
sky_root getSkyRoot(const char *fb, size_t fb_size);
sky_rec getSkyRec(const Tables::Record *rec);

// print functions (debug only)
void printSkyRoot(sky_root *r);
void printSkyRec(sky_rec *r);
void printSkyFb(const char* fb, size_t fb_size);
long long int printFlatbufFlexRowAsCsv(const char* dataptr,
                                       const size_t datasz,
                                       bool print_header,
                                       bool print_verbose,
                                       long long int max_to_print);

// convert provided schema to/from skyhook internal representation
schema_vec schemaFromColNames(schema_vec &current_schema,
                              std::string project_col_names);
schema_vec schemaFromString(std::string schema_string);
std::string schemaToString(schema_vec schema);

// convert provided predicates to/from skyhook internal representation
predicate_vec predsFromString(schema_vec &schema, std::string preds_string);
std::string predsToString(predicate_vec &preds,  schema_vec &schema);
std::vector<std::string> colnamesFromPreds(predicate_vec &preds,
                                           schema_vec &schema);
std::vector<std::string> colnamesFromSchema(schema_vec &schema);

bool hasAggPreds(predicate_vec &preds);

// convert provided ops to/from internal skyhook representation (simple enums)
int skyOpTypeFromString(std::string s);
std::string skyOpTypeToString(int op);

// for proj, select, fastpath, aggregations: process data and build return fb
int processSkyFb(
        flatbuffers::FlatBufferBuilder& flatb,
        schema_vec& data_schema,
        schema_vec& query_schema,
        predicate_vec& preds,
        const char* fb,
        const size_t fb_size,
        std::string& errmsg,
        const std::vector<uint32_t>& row_nums=std::vector<uint32_t>());

inline
bool applyPredicates(predicate_vec& pv, sky_rec& rec);

inline
bool compare(const int64_t& val1, const int64_t& val2, const int& op);

inline
bool compare(const uint64_t& val1, const uint64_t& val2, const int& op);

inline
bool compare(const double& val1, const double& val2, const int& op);

inline
bool compare(const bool& val1, const bool& val2, const int& op);

inline  // used for date types or regex on alphanumeric types
bool compare(const std::string& val1, const std::string& val2, const int& op, const int& data_type);

template <typename T>
T computeAgg(const T& val, const T& oldval, const int& op) {

    switch (op) {
        case SOT_min: if (val < oldval) return val;
        case SOT_max: if (val > oldval) return val;
        case SOT_sum: return oldval + val;
        case SOT_cnt: return oldval + 1;
        default: assert (TablesErrCodes::OpNotImplemented);
    }
    return oldval;
}

std::string buildKeyPrefix(
        int idx_type,
        std::string schema_name,
        std::string table_name,
        std::vector<string> colnames=std::vector<string>());
std::string buildKeyData(int data_type, uint64_t new_data);

// used for index prefix matching during index range queries
bool compare_keys(std::string key1, std::string key2);

// used for matching lt/leq index predicates
bool check_predicate_ops(predicate_vec index_preds, int opType);
bool check_predicate_ops_all_include_equality(predicate_vec index_preds);
bool check_predicate_ops_all_equality(predicate_vec index_preds);

// for extracting the typed value from a typed predicate then
// returning as uint64, which is then used to build a key-data value string
void extract_typedpred_val(Tables::PredicateBase* pb, uint64_t& val);
void extract_typedpred_val(Tables::PredicateBase* pb, int64_t& val);

} // end namespace Tables


#endif
