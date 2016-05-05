# Faultload structure #

```
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
```

| **Opcode** | **Information** |
|:-----------|:----------------|
| COMMAND    |                 |
| AFTER      |                 |
| WHILE      | Not implemented |
| HOST       |                 |
| IF         |                 |
| ELSE       |                 |
| END\_IF    |                 |
| END        |                 |
| SET        | Not implemented |
| FOREACH    |                 |
| PARTITION  |                 |
| DECLARE    |                 |