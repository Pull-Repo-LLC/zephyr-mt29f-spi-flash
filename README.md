# MT29F SPI Device for Zephyr RTOS

Basic SPI driver for mt29f NAND part series.

- *Note: Only supports page-aligned access at this time.*

## Contents
*/src* - contains the driver implementation.

*/example* - contains example application usage. It also contains the required DTS yaml file for out-of-tree inclusion in the Zephyr ecosystem. Finally, an example dts entry for your board is included in [sample.dts](example/sample.dts).

## License

There are no claims or guarantees of correctness of this implementation. Please see [LICENSE](LICENSE) file for usage information.
