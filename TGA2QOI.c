#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>

#ifndef FREE
#define FREE(p) { if(p) { free(p); p=NULL; } }
#endif

typedef struct
{
	uint32_t Width;
	uint32_t Height;
	uint32_t Depth;
	uint8_t *Data;
} Image_t;

void TGA_GetRLE(unsigned char *data, int width, int height, int depth, FILE *stream)
{
	int current_byte=0, size, run_length, i;
	unsigned char header, numbytes, buffer[4];

	numbytes=depth>>3;
	size=width*height*numbytes;

	while(current_byte<size)
	{
		fread(&header, sizeof(unsigned char), 1, stream);

		run_length=(header&0x7F)+1;

		if(header&0x80)
		{
			fread(buffer, sizeof(unsigned char), numbytes, stream);

			for(i=0;i<run_length;i++)
			{
				memcpy(&data[current_byte], buffer, numbytes);
				current_byte+=numbytes;
			}
		}
		else
		{
			for(i=0;i<run_length;i++)
			{
				fread(&data[current_byte], sizeof(unsigned char), numbytes, stream);
				current_byte+=numbytes;
			}
		}
	}
}

int TGA_Load(const char *Filename, Image_t *Image)
{
	FILE *stream=NULL;
	uint8_t IDLength;
	uint8_t ColorMapType, ImageType;
	uint16_t ColorMapStart, ColorMapLength;
	uint8_t ColorMapDepth;
	uint16_t XOffset, YOffset;
	uint16_t Width, Height;
	uint8_t Depth;
	uint8_t ImageDescriptor;
	int i;

	if((stream=fopen(Filename, "rb"))==NULL)
		return 0;

	fread(&IDLength, sizeof(uint8_t), 1, stream);
	fread(&ColorMapType, sizeof(uint8_t), 1, stream);
	fread(&ImageType, sizeof(uint8_t), 1, stream);
	fread(&ColorMapStart, sizeof(uint16_t), 1, stream);
	fread(&ColorMapLength, sizeof(uint16_t), 1, stream);
	fread(&ColorMapDepth, sizeof(uint8_t), 1, stream);
	fread(&XOffset, sizeof(uint16_t), 1, stream);
	fread(&YOffset, sizeof(uint16_t), 1, stream);
	fread(&Width, sizeof(uint16_t), 1, stream);
	fread(&Height, sizeof(uint16_t), 1, stream);
	fread(&Depth, sizeof(uint8_t), 1, stream);
	fread(&ImageDescriptor, sizeof(uint8_t), 1, stream);
	fseek(stream, IDLength, SEEK_CUR);

	switch(ImageType)
	{
		case 11:
		case 10:
		case 9:
		case 3:
		case 2:
		case 1:
			break;

		default:
			fclose(stream);
			return 0;
	}

	switch(Depth)
	{
		case 32:
			Image->Data=(uint8_t *)malloc(Width*Height*4);

			if(Image->Data==NULL)
				return 0;

			if(ImageType==10)
				TGA_GetRLE(Image->Data, Width, Height, Depth, stream);
			else
				fread(Image->Data, sizeof(uint8_t), Width*Height*4, stream);
			break;

		case 24:
			Image->Data=(uint8_t *)malloc(Width*Height*3);

			if(Image->Data==NULL)
				return 0;

			if(ImageType==10)
				TGA_GetRLE(Image->Data, Width, Height, Depth, stream);
			else
				fread(Image->Data, sizeof(uint8_t), Width*Height*3, stream);
			break;

		case 16:
		case 15:
			Image->Data=(uint8_t *)malloc(sizeof(uint16_t)*Width*Height);

			if(Image->Data==NULL)
				return 0;

			if(ImageType==10)
				TGA_GetRLE(Image->Data, Width, Height, Depth, stream);
			else
				fread(Image->Data, sizeof(uint16_t), Width*Height, stream);

			Depth=16;
			break;

		case 8:
			if(ColorMapType)
			{
				uint8_t *ColorMap=NULL, *Buffer=NULL;

				// Ugh
				switch(ColorMapDepth)
				{
					case 32:
						ColorMap=(uint8_t *)malloc(ColorMapLength*4);

						if(ColorMap==NULL)
							return 0;

						fread(ColorMap, sizeof(uint8_t), ColorMapLength*4, stream);

						Buffer=(uint8_t *)malloc(Width*Height);
						
						if(Buffer==NULL)
						{
							FREE(Buffer);
							return 0;
						}
						
						if(ImageType==9)
							TGA_GetRLE(Buffer, Width, Height, Depth, stream);
						else
							fread(Buffer, sizeof(uint8_t), Width*Height, stream);

						Image->Data=(uint8_t *)malloc(Width*Height*4);

						if(Image->Data==NULL)
						{
							FREE(Buffer);
							FREE(ColorMap);

							return 0;
						}

						for(i=0;i<Width*Height;i++)
						{
							Image->Data[4*i+0]=ColorMap[4*Buffer[i]+0];
							Image->Data[4*i+1]=ColorMap[4*Buffer[i]+1];
							Image->Data[4*i+2]=ColorMap[4*Buffer[i]+2];
							Image->Data[4*i+4]=ColorMap[4*Buffer[i]+3];
						}

						Depth=32;
						break;

					case 24:
						ColorMap=(uint8_t *)malloc(ColorMapLength*3);

						if(ColorMap==NULL)
							return 0;

						fread(ColorMap, sizeof(uint8_t), ColorMapLength*3, stream);

						Buffer=(uint8_t *)malloc(Width*Height);

						if(Buffer==NULL)
						{
							FREE(ColorMap);
							return 0;
						}
						
						if(ImageType==9)
							TGA_GetRLE(Buffer, Width, Height, Depth, stream);
						else
							fread(Buffer, sizeof(uint8_t), Width*Height, stream);

						Image->Data=(uint8_t *)malloc(Width*Height*3);

						if(Image->Data==NULL)
						{
							FREE(Buffer);
							FREE(ColorMap);
							return 0;
						}

						for(i=0;i<Width*Height;i++)
						{
							Image->Data[3*i+0]=ColorMap[3*Buffer[i]+0];
							Image->Data[3*i+1]=ColorMap[3*Buffer[i]+1];
							Image->Data[3*i+2]=ColorMap[3*Buffer[i]+2];
						}

						Depth=24;
						break;

					case 16:
					case 15:
						ColorMap=(uint8_t *)malloc(sizeof(uint16_t)*ColorMapLength);

						if(ColorMap==NULL)
						{
							FREE(Buffer);
							return 0;
						}

						fread(ColorMap, sizeof(uint16_t), ColorMapLength, stream);

						Buffer=(uint8_t *)malloc(Width*Height);

						if(Buffer==NULL)
						{
							FREE(Buffer);
							return 0;
						}
						
						if(ImageType==9)
							TGA_GetRLE(Buffer, Width, Height, Depth, stream);
						else
							fread(Buffer, sizeof(uint8_t), Width*Height, stream);

						Image->Data=(uint8_t *)malloc(sizeof(uint16_t)*Width*Height);

						if(Image->Data==NULL)
						{
							FREE(Buffer);
							FREE(ColorMap);
							return 0;
						}

						for(i=0;i<Width*Height;i++)
							((uint16_t *)Image->Data)[i]=((uint16_t *)ColorMap)[Buffer[i]];

						Depth=16;
						break;
				}

				FREE(Buffer);
				FREE(ColorMap);
			}
			else
			{
				Image->Data=(uint8_t *)malloc(Width*Height);

				if(Image->Data==NULL)
					return 0;

				if(ImageType==11)
					TGA_GetRLE(Image->Data, Width, Height, Depth, stream);
				else
					fread(Image->Data, sizeof(uint8_t), Width*Height, stream);
			}
			break;

		default:
			fclose(stream);
			return 0;
	}

	fclose(stream);

	// This might have been wrong for 20+ years?
	// It was:
	// if(ImageDescriptor&0x20)
	if(!(ImageDescriptor&0x20))
	{
		int Scanline=Width*(Depth>>3), Size=Scanline*Height;
		uint8_t *Buffer=(uint8_t *)malloc(Size);

		if(Buffer==NULL)
		{
			FREE(Image->Data);
			return 0;
		}

		for(i=0;i<Height;i++)
			memcpy(Buffer+(Size-(i+1)*Scanline), Image->Data+i*Scanline, Scanline);

		memcpy(Image->Data, Buffer, Size);

		FREE(Buffer);
	}

	Image->Width=Width;
	Image->Height=Height;
	Image->Depth=Depth;

	return 1;
}

#define QOI_MAGIC			(((uint32_t)'q')<<24|((uint32_t)'o')<<16|((uint32_t)'i')<<8|((uint32_t)'f'))

#define QOI_SRGB			0
#define QOI_LINEAR			1

#define QOI_OP_INDEX		0x00
#define QOI_OP_DIFF			0x40
#define QOI_OP_LUMA			0x80
#define QOI_OP_RUN			0xC0
#define QOI_OP_RGB			0xFE
#define QOI_OP_RGBA			0xFF
#define QOI_OP_MASK			0xC0

#define QOI_HASH(C)	(C[0]*3+C[1]*5+C[2]*7+C[3]*11)

const uint8_t qoi_padding[8]={ 0, 0, 0, 0, 0, 0, 0, 1 };

uint32_t Int32Swap(const uint32_t l)
{
	uint8_t b1=(l>>0 )&255;
	uint8_t b2=(l>>8 )&255;
	uint8_t b3=(l>>16)&255;
	uint8_t b4=(l>>24)&255;

	return ((uint32_t)b1<<24)+((uint32_t)b2<<16)+((uint32_t)b3<<8)+((uint32_t)b4<<0);
}

int32_t QOI_Write(const char *Filename, Image_t *Image)
{
	FILE *stream=NULL;
	int run=0;
	uint8_t index[64][4], px[4], px_prev[4];
	uint8_t temp8, channels;
	uint32_t temp32, i;

	if(Image==NULL)
		return 0;

	if(!(stream=fopen(Filename, "wb")))
		return 0;

	// MAGIC
	temp32=Int32Swap(QOI_MAGIC);
	fwrite(&temp32, 1, sizeof(uint32_t), stream);

	// Width
	temp32=Int32Swap(Image->Width);
	fwrite(&temp32, 1, sizeof(uint32_t), stream);

	// Height
	temp32=Int32Swap(Image->Height);
	fwrite(&temp32, 1, sizeof(uint32_t), stream);

	// Channels
	channels=Image->Depth>>3;
	fwrite(&channels, 1, sizeof(uint8_t), stream);

	// Colorspace
	temp8=QOI_SRGB;
	fwrite(&temp8, 1, sizeof(uint8_t), stream);

	memset(index, 0, sizeof(index));

	px_prev[0]=0;
	px_prev[1]=0;
	px_prev[2]=0;
	px_prev[3]=255;

	px[0]=0;
	px[1]=0;
	px[2]=0;
	px[3]=255;

	for(i=0;i<Image->Width*Image->Height*channels;i+=channels)
	{
		px[0]=Image->Data[i+2];
		px[1]=Image->Data[i+1];
		px[2]=Image->Data[i+0];

		if(channels==4)
		{
			px[3]=Image->Data[i+3];
		}

		if(px[0]==px_prev[0]&&px[1]==px_prev[1]&&px[2]==px_prev[2]&&px[3]==px_prev[3])
		{
			run++;

			if(run==62)
			{
				temp8=QOI_OP_RUN|(run-1);
				fwrite(&temp8, 1, sizeof(uint8_t), stream);
				run=0;
			}
		}
		else
		{
			int index_pos;

			if(run>0)
			{
				temp8=QOI_OP_RUN|(run-1);
				fwrite(&temp8, 1, sizeof(uint8_t), stream);
				run=0;
			}

			index_pos=QOI_HASH(px)%64;

			if(index[index_pos][0]==px[0]&&index[index_pos][1]==px[1]&&index[index_pos][2]==px[2]&&index[index_pos][3]==px[3])
			{
				temp8=QOI_OP_INDEX|index_pos;
				fwrite(&temp8, 1, sizeof(uint8_t), stream);
			}
			else
			{
				index[index_pos][0]=px[0];
				index[index_pos][1]=px[1];
				index[index_pos][2]=px[2];
				index[index_pos][3]=px[3];

				if(px[3]==px_prev[3])
				{
					int8_t vr=px[0]-px_prev[0];
					int8_t vg=px[1]-px_prev[1];
					int8_t vb=px[2]-px_prev[2];

					int8_t vg_r=vr-vg;
					int8_t vg_b=vb-vg;

					if(vr>-3&&vr<2&&vg>-3&&vg<2&&vb>-3&&vb<2)
					{
						temp8=QOI_OP_DIFF|(vr+2)<<4|(vg+2)<<2|(vb+2);
						fwrite(&temp8, 1, sizeof(uint8_t), stream);
					}
					else if(vg_r>-9&&vg_r<8&&vg>-33&&vg<32&&vg_b>-9&&vg_b<8)
					{
						temp8=QOI_OP_LUMA|(vg+32);
						fwrite(&temp8, 1, sizeof(uint8_t), stream);
						temp8=(vg_r+8)<<4|(vg_b+8);
						fwrite(&temp8, 1, sizeof(uint8_t), stream);
					}
					else
					{
						temp8=QOI_OP_RGB;
						fwrite(&temp8, 1, sizeof(uint8_t), stream);
						fwrite(&px[0], 1, sizeof(uint8_t), stream);
						fwrite(&px[1], 1, sizeof(uint8_t), stream);
						fwrite(&px[2], 1, sizeof(uint8_t), stream);
					}
				}
				else
				{
					temp8=QOI_OP_RGBA;
					fwrite(&temp8, 1, sizeof(uint8_t), stream);
					fwrite(&px[0], 1, sizeof(uint8_t), stream);
					fwrite(&px[1], 1, sizeof(uint8_t), stream);
					fwrite(&px[2], 1, sizeof(uint8_t), stream);
					fwrite(&px[3], 1, sizeof(uint8_t), stream);
				}
			}
		}

		px_prev[0]=px[0];
		px_prev[1]=px[1];
		px_prev[2]=px[2];
		px_prev[3]=px[3];
	}

	fwrite(qoi_padding, 1, sizeof(qoi_padding), stream);

	fclose(stream);

	return 1;
}

int main(int argc, char **argv)
{
	FILE *stream=NULL;
	Image_t Image;
	char buf[256];
	char *ptr;

	if(argc<2)
	{
		fprintf(stderr, "Usage:\n%s in.tga\n", argv[0]);
		return -1;
	}

	if(!TGA_Load(argv[1], &Image))
	{
		fprintf(stderr, "Unable to open %s for input.\n", argv[1]);
		return -1;
	}

	if(Image.Depth<24)
	{
		fprintf(stderr, "Only 24 and 32bit images are supported.\n");
		FREE(Image.Data);
		return -1;
	}

	strncpy(buf, argv[1], 256);
	ptr=strstr(buf, ".");
	ptr[0]='\0';
	strcat(buf, ".qoi");

	QOI_Write(buf, &Image);

	FREE(Image.Data);

	return 0;
}