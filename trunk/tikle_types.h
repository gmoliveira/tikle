#ifndef TIKLE_TYPES_H
#define TIKLE_TYPES_H

/**
 * Actions
 */
typedef enum {
	ACT_DELAY = 0,
	ACT_DUPLICATE,
	ACT_DROP
} faultload_action;

/**
 * Protocols
 */
typedef enum  {
	ALL_PROTOCOL = 0,
	TCP_PROTOCOL = 6,
	UDP_PROTOCOL = 17,
	SCTP_PROTOCOL = 132
} faultload_protocol;

/**
 * Numeric representation type
 * 1  -> NONE
 * 1s -> TEMPORAL
 * 1p -> NPACKETS
 * 1% -> PERCT
 */
typedef enum _faultload_num_type {
	NONE = 0,
	TEMPORAL,
	NPACKETS,
	PERCT
} faultload_num_type;

/**
 * Opcode data type structure
 */
typedef union {
	unsigned long num;
	struct _str_value {
		size_t length;
		char *value;
	} str;
	struct _array_value {
		size_t count;
		unsigned long nums[10];
	} array;
} faultload_value_type;

/**
 * Opcode type
 */
typedef enum {
	COMMAND,
	AFTER,
	WHILE,
	HOST,
	IF,
	ELSE,
	END_IF,
	END,
	SET,
	FOREACH,
	PARTITION,
	DECLARE
} faultload_opcode;

/**
 * Op data type
 */
typedef enum {
	UNUSED = 0,
	NUMBER,
	STRING,
	VAR,
	ARRAY
} faultload_op_type;

/**
 * Opcode structure
 */
typedef struct {
	faultload_opcode opcode;
	faultload_protocol protocol;
	faultload_num_type occur_type;
	unsigned short int num_ops;
	faultload_op_type *op_type;
	faultload_value_type *op_value;
	int extended_value;
	unsigned long label;
	short int block_type;
	unsigned int next_op;
} faultload_op;

#endif /* TIKLE_TYPES_H */
