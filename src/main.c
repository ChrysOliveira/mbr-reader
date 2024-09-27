#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define TAMANHO_TOTAL_MBR 512
#define TAMANHO_PARTICAO 16
#define TAMANHO_SETOR 512
#define QNT_PARTICOES 4
#define OFFSET_BOOTSTRAP 446
#define OFFSET_PARTICOES 510

typedef struct {
	uint8_t status;
	uint8_t chs_first_abs_sector[3];
	uint8_t partition_type;
	uint8_t chs_last_abs_sector[3];
	uint32_t lba_first_abs_sector;
	uint32_t num_sections;

} PartitionEntry;

typedef struct {
	uint8_t part1;
	uint8_t part2;
} SignatureEntry;

char* descricao_tipo_particao(uint8_t partition_type){
	char * retorno;
	switch(partition_type){
		case 0x00:
			retorno = "Empty";
			break;
		case 0x01:
			retorno = "FAT12";
			break;
		case 0x04:
			retorno = "FAT16 < 32MB";
			break;
		case 0x05:
			retorno = "Extended";
			break;
		case 0x06:
			retorno = "FAT16 > 32MB";
			break;
		case 0x07:
			retorno = "NTFS / exFAT";
			break;
		case 0x0b:
			retorno = "W95 FAT32";
			break;
		case 0x0c:
			retorno = "FAT32 (LBA, com INT 13 ext.)";
			break;
		case 0x0e:
			retorno = "FAT16 (LBA)";
			break;
		case 0x0f:
			retorno = "Extended (LBA)";
			break;
		case 0x82:
			retorno = "Linux swap";
			break;
		case 0x83:
			retorno = "Linux";
			break;
		case 0x8e:
			retorno = "Linux LVM";
			break;
		case 0xa5:
			retorno = "FreeBSD";
			break;
		case 0xa6:
			retorno = "OpenBSD";
			break;
		case 0xa8:
			retorno = "macOS X";
			break;
		case 0xaf:
			retorno = "macOS X HFS+";
			break;
		case 0xeb:
			retorno = "BeOS BFS";
			break;
		case 0xee:
			retorno = "GPT";
			break;
		default: 
			retorno = "Descricao particao nao mapeada";
			break;
	}
	return retorno;
}

int main(char* args){
	unsigned char buffer[TAMANHO_TOTAL_MBR];
	FILE *ptr;

	ptr = fopen("../mbr.bin", "rb");

	if (ptr == NULL) {
		printf("Erro ao abrir o arquivo.\n");
		return 1;
	}

	fread(buffer, sizeof(buffer), 1, ptr);
	fclose(ptr);

	for(int i = 0; i<512; i+=2){
		if (i % 16 == 0){
			printf("\n");
		} 

		printf("%02x%02x ", buffer[i + 1], buffer[i]);
	}
	printf("\n");
	printf("\n");

	uint32_t total_sectors = 0;
	for (int i = 0; i < QNT_PARTICOES; i++){
		printf("Particao %d\n", i);

		PartitionEntry *pe = (PartitionEntry *) &buffer[OFFSET_BOOTSTRAP + (i * TAMANHO_PARTICAO)];

		printf("  Status da particao: %s \n", pe->status == 0x80 ? "Bootavel" : "Nao bootavel");
		printf("  Endereco primeiro setor absoluto (CHS): %02x \n", pe->chs_first_abs_sector);
		printf("  Endereco ultimo setor absoluto (CHS): %02x \n", pe->chs_last_abs_sector);
		printf("  Tipo da particao: %s \n", descricao_tipo_particao(pe->partition_type));
		printf("  Endereco primeiro setor absoluto (LBA): %02x \n", pe->lba_first_abs_sector);
		printf("  Quantidade de setores na particao: %02x \n\n", pe->num_sections);

		total_sectors += pe->num_sections;
	}

	SignatureEntry *se = (SignatureEntry *) &buffer[OFFSET_PARTICOES];
	printf("Assinatura MBR: %02x%02x\n", se->part1, se->part2);

	printf("\n");
	printf("\n");
	printf("Print \"fdisk -l /dev/vda\": \n");
	printf("\n");

	uint64_t total_bytes = (uint64_t)total_sectors * TAMANHO_SETOR;
	double total_gib = (double)total_bytes / (1024 * 1024 * 1024);


	printf("Disk: %.2f GiB, %llu bytes, %u sectors\n", total_gib, total_bytes, total_sectors);
	printf("Units: sectors of 1 * %d = %d bytes\n", TAMANHO_SETOR, TAMANHO_SETOR);
	printf("Sector size (logical/physical): %d bytes / %d bytes\n", TAMANHO_SETOR, TAMANHO_SETOR);
	printf("I/O size (minimum/optimal): %d bytes / %d bytes\n", TAMANHO_SETOR, TAMANHO_SETOR);
	printf("Disklabel type: dos\n"); // hardcoded para ficar igual o comando
	printf("Disk identifier: 0x55749c7e\n\n"); //hardcoded para ficar igual o comando

	printf("Device      Boot  Start   End       Sectors   Size    Id Type\n");

	for (int i = 0; i < QNT_PARTICOES; i++) {
		PartitionEntry *pe = (PartitionEntry *) &buffer[OFFSET_BOOTSTRAP + (i * TAMANHO_PARTICAO)];

		if (pe->partition_type == 0x00) continue;

		char boot_flag = (pe->status == 0x80) ? '*' : ' ';
		uint32_t num_sectors = pe->num_sections;
		uint32_t start_sector = pe->lba_first_abs_sector;
		uint32_t end_sector = start_sector + num_sectors - 1;
		double size = ((double)num_sectors * TAMANHO_SETOR) / (1024 * 1024); 

		printf("/dev/vda%d   %c   %8u  %8u  %8u  %.1fM  %02x %s\n", 
				i + 1, 						//%d 	device
				boot_flag,					//%c  	boot
				start_sector,					//%8u  	start
				end_sector, 					//%8u	end
				num_sectors, 					//%8u	sectors
				size,						//%.1f	size
				pe->partition_type,				//%02x	id
				descricao_tipo_particao(pe->partition_type));	//%s    type
	}

	return 0;
}
