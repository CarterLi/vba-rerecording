// RomInfo.cpp : implementation file
//

#include "../stdafx.h"
#include "../resource.h"
#include "RomInfo.h"
#include "../WinResUtil.h"

extern int32 gbRomSize;

struct WinGBACompanyName
{
	LPCTSTR code;
	LPCTSTR name;
};

static WinGBACompanyName winGBARomInfoCompanies[] = {
	{ _T("01"), _T("Nintendo")							  },
	{ _T("02"), _T("Rocket Games")						  },
	{ _T("08"), _T("Capcom")							  },
	{ _T("09"), _T("Hot B Co.")							  },
	{ _T("0A"), _T("Jaleco")							  },
	{ _T("0B"), _T("Coconuts Japan")					  },
	{ _T("0C"), _T("Coconuts Japan/G.X.Media")			  },
	{ _T("0H"), _T("Starfish")							  },
	{ _T("0L"), _T("Warashi Inc.")						  },
	{ _T("0N"), _T("Nowpro")							  },
	{ _T("0P"), _T("Game Village")						  },
	{ _T("13"), _T("Electronic Arts Japan")				  },
	{ _T("18"), _T("Hudson Soft Japan")					  },
	{ _T("19"), _T("S.C.P.")							  },
	{ _T("1A"), _T("Yonoman")							  },
	{ _T("1G"), _T("SMDE")								  },
	{ _T("1P"), _T("Creatures Inc.")					  },
	{ _T("1Q"), _T("TDK Deep Impresion")				  },
	{ _T("20"), _T("Destination Software")				  },
	{ _T("22"), _T("VR 1 Japan")						  },
	{ _T("25"), _T("San-X")								  },
	{ _T("28"), _T("Kemco Japan")						  },
	{ _T("29"), _T("Seta")								  },
	{ _T("2H"), _T("Ubisoft Japan")						  },
	{ _T("2K"), _T("NEC InterChannel")					  },
	{ _T("2L"), _T("Tam")								  },
	{ _T("2M"), _T("Jordan")							  },
	{ _T("2N"), _T("Smilesoft")							  },
	{ _T("2Q"), _T("Mediakite")							  },
	{ _T("36"), _T("Codemasters")						  },
	{ _T("37"), _T("GAGA Communications")				  },
	{ _T("38"), _T("Laguna")							  },
	{ _T("39"), _T("Telstar Fun and Games")				  },
	{ _T("41"), _T("Ubi Soft Entertainment")			  },
	{ _T("42"), _T("Sunsoft")							  },
	{ _T("47"), _T("Spectrum Holobyte")					  },
	{ _T("49"), _T("IREM")								  },
	{ _T("4D"), _T("Malibu Games")						  },
	{ _T("4F"), _T("Eidos/U.S. Gold")					  },
	{ _T("4J"), _T("Fox Interactive")					  },
	{ _T("4K"), _T("Time Warner Interactive")			  },
	{ _T("4Q"), _T("Disney")							  },
	{ _T("4S"), _T("Black Pearl")						  },
	{ _T("4X"), _T("GT Interactive")					  },
	{ _T("4Y"), _T("RARE")								  },
	{ _T("4Z"), _T("Crave Entertainment")				  },
	{ _T("50"), _T("Absolute Entertainment")			  },
	{ _T("51"), _T("Acclaim")							  },
	{ _T("52"), _T("Activision")						  },
	{ _T("53"), _T("American Sammy Corp.")				  },
	{ _T("54"), _T("Take 2 Interactive")				  },
	{ _T("55"), _T("Hi Tech")							  },
	{ _T("56"), _T("LJN LTD.")							  },
	{ _T("58"), _T("Mattel")							  },
	{ _T("5A"), _T("Mindscape/Red Orb Ent.")			  },
	{ _T("5C"), _T("Taxan")								  },
	{ _T("5D"), _T("Midway")							  },
	{ _T("5F"), _T("American Softworks")				  },
	{ _T("5G"), _T("Majesco Sales Inc")					  },
	{ _T("5H"), _T("3DO")								  },
	{ _T("5K"), _T("Hasbro")							  },
	{ _T("5L"), _T("NewKidCo")							  },
	{ _T("5M"), _T("Telegames")							  },
	{ _T("5N"), _T("Metro3D")							  },
	{ _T("5P"), _T("Vatical Entertainment")				  },
	{ _T("5Q"), _T("LEGO Media")						  },
	{ _T("5S"), _T("Xicat Interactive")					  },
	{ _T("5T"), _T("Cryo Interactive")					  },
	{ _T("5W"), _T("Red Storm Ent./BKN Ent.")			  },
	{ _T("5X"), _T("Microids")							  },
	{ _T("5Z"), _T("Conspiracy Entertainment Corp.")	  },
	{ _T("60"), _T("Titus Interactive Studios")			  },
	{ _T("61"), _T("Virgin Interactive")				  },
	{ _T("62"), _T("Maxis")								  },
	{ _T("64"), _T("LucasArts Entertainment")			  },
	{ _T("67"), _T("Ocean")								  },
	{ _T("69"), _T("Electronic Arts")					  },
	{ _T("6E"), _T("Elite Systems Ltd.")				  },
	{ _T("6F"), _T("Electro Brain")						  },
	{ _T("6G"), _T("The Learning Company")				  },
	{ _T("6H"), _T("BBC")								  },
	{ _T("6J"), _T("Software 2000")						  },
	{ _T("6L"), _T("BAM! Entertainment")				  },
	{ _T("6M"), _T("Studio 3")							  },
	{ _T("6Q"), _T("Classified Games")					  },
	{ _T("6S"), _T("TDK Mediactive")					  },
	{ _T("6U"), _T("DreamCatcher")						  },
	{ _T("6V"), _T("JoWood Productions")				  },
	{ _T("6W"), _T("SEGA")								  },
	{ _T("6X"), _T("Wannado Edition")					  },
	{ _T("6Y"), _T("LSP")								  },
	{ _T("6Z"), _T("ITE Media")							  },
	{ _T("70"), _T("Infogrames")						  },
	{ _T("71"), _T("Interplay")							  },
	{ _T("72"), _T("JVC Musical Industries Inc")		  },
	{ _T("73"), _T("Parker Brothers")					  },
	{ _T("75"), _T("SCI")								  },
	{ _T("78"), _T("THQ")								  },
	{ _T("79"), _T("Accolade")							  },
	{ _T("7A"), _T("Triffix Ent. Inc.")					  },
	{ _T("7C"), _T("Microprose Software")				  },
	{ _T("7D"), _T("Universal Interactive Studios")		  },
	{ _T("7F"), _T("Kemco")								  },
	{ _T("7G"), _T("Rage Software")						  },
	{ _T("7H"), _T("Encore")							  },
	{ _T("7J"), _T("Zoo")								  },
	{ _T("7K"), _T("BVM")								  },
	{ _T("7L"), _T("Simon & Schuster Interactive")		  },
	{ _T("7M"), _T("Asmik Ace Entertainment Inc./AIA")	  },
	{ _T("7N"), _T("Empire Interactive")				  },
	{ _T("7Q"), _T("Jester Interactive")				  },
	{ _T("7T"), _T("Scholastic")						  },
	{ _T("7U"), _T("Ignition Entertainment")			  },
	{ _T("7W"), _T("Stadlbauer")						  },
	{ _T("80"), _T("Misawa")							  },
	{ _T("83"), _T("LOZC")								  },
	{ _T("8B"), _T("Bulletproof Software")				  },
	{ _T("8C"), _T("Vic Tokai Inc.")					  },
	{ _T("8J"), _T("General Entertainment")				  },
	{ _T("8N"), _T("Success")							  },
	{ _T("8P"), _T("SEGA Japan")						  },
	{ _T("91"), _T("Chun Soft")							  },
	{ _T("92"), _T("Video System")						  },
	{ _T("93"), _T("BEC")								  },
	{ _T("96"), _T("Yonezawa/S'pal")					  },
	{ _T("97"), _T("Kaneko")							  },
	{ _T("99"), _T("Victor Interactive Software")		  },
	{ _T("9A"), _T("Nichibutsu/Nihon Bussan")			  },
	{ _T("9B"), _T("Tecmo")								  },
	{ _T("9C"), _T("Imagineer")							  },
	{ _T("9F"), _T("Nova")								  },
	{ _T("9H"), _T("Bottom Up")							  },
	{ _T("9L"), _T("Hasbro Japan")						  },
	{ _T("9N"), _T("Marvelous Entertainment")			  },
	{ _T("9P"), _T("Keynet Inc.")						  },
	{ _T("9Q"), _T("Hands-On Entertainment")			  },
	{ _T("A0"), _T("Telenet")							  },
	{ _T("A1"), _T("Hori")								  },
	{ _T("A4"), _T("Konami")							  },
	{ _T("A6"), _T("Kawada")							  },
	{ _T("A7"), _T("Takara")							  },
	{ _T("A9"), _T("Technos Japan Corp.")				  },
	{ _T("AA"), _T("JVC")								  },
	{ _T("AC"), _T("Toei Animation")					  },
	{ _T("AD"), _T("Toho")								  },
	{ _T("AF"), _T("Namco")								  },
	{ _T("AG"), _T("Media Rings Corporation")			  },
	{ _T("AH"), _T("J-Wing")							  },
	{ _T("AK"), _T("KID")								  },
	{ _T("AL"), _T("MediaFactory")						  },
	{ _T("AP"), _T("Infogrames Hudson")					  },
	{ _T("AQ"), _T("Kiratto. Ludic Inc")				  },
	{ _T("B0"), _T("Acclaim Japan")						  },
	{ _T("B1"), _T("ASCII")								  },
	{ _T("B2"), _T("Bandai")							  },
	{ _T("B4"), _T("Enix")								  },
	{ _T("B6"), _T("HAL Laboratory")					  },
	{ _T("B7"), _T("SNK")								  },
	{ _T("B9"), _T("Pony Canyon Hanbai")				  },
	{ _T("BA"), _T("Culture Brain")						  },
	{ _T("BB"), _T("Sunsoft")							  },
	{ _T("BD"), _T("Sony Imagesoft")					  },
	{ _T("BF"), _T("Sammy")								  },
	{ _T("BG"), _T("Magical")							  },
	{ _T("BJ"), _T("Compile")							  },
	{ _T("BL"), _T("MTO Inc.")							  },
	{ _T("BN"), _T("Sunrise Interactive")				  },
	{ _T("BP"), _T("Global A Entertainment")			  },
	{ _T("BQ"), _T("Fuuki")								  },
	{ _T("C0"), _T("Taito")								  },
	{ _T("C2"), _T("Kemco")								  },
	{ _T("C3"), _T("Square Soft")						  },
	{ _T("C5"), _T("Data East")							  },
	{ _T("C6"), _T("Tonkin House")						  },
	{ _T("C8"), _T("Koei")								  },
	{ _T("CA"), _T("Konami/Palcom/Ultra")				  },
	{ _T("CB"), _T("Vapinc/NTVIC")						  },
	{ _T("CC"), _T("Use Co.,Ltd.")						  },
	{ _T("CD"), _T("Meldac")							  },
	{ _T("CE"), _T("FCI/Pony Canyon")					  },
	{ _T("CF"), _T("Angel")								  },
	{ _T("CM"), _T("Konami Computer Entertainment Osaka") },
	{ _T("CP"), _T("Enterbrain")						  },
	{ _T("D1"), _T("Sofel")								  },
	{ _T("D2"), _T("Quest")								  },
	{ _T("D3"), _T("Sigma Enterprises")					  },
	{ _T("D4"), _T("Ask Kodansa")						  },
	{ _T("D6"), _T("Naxat")								  },
	{ _T("D7"), _T("Copya System")						  },
	{ _T("D9"), _T("Banpresto")							  },
	{ _T("DA"), _T("TOMY")								  },
	{ _T("DB"), _T("LJN Japan")							  },
	{ _T("DD"), _T("NCS")								  },
	{ _T("DF"), _T("Altron Corporation")				  },
	{ _T("DH"), _T("Gaps Inc.")							  },
	{ _T("DN"), _T("ELF")								  },
	{ _T("E2"), _T("Yutaka")							  },
	{ _T("E3"), _T("Varie")								  },
	{ _T("E5"), _T("Epoch")								  },
	{ _T("E7"), _T("Athena")							  },
	{ _T("E8"), _T("Asmik Ace Entertainment Inc.")		  },
	{ _T("E9"), _T("Natsume")							  },
	{ _T("EA"), _T("King Records")						  },
	{ _T("EB"), _T("Atlus")								  },
	{ _T("EC"), _T("Epic/Sony Records")					  },
	{ _T("EE"), _T("IGS")								  },
	{ _T("EL"), _T("Spike")								  },
	{ _T("EM"), _T("Konami Computer Entertainment Tokyo") },
	{ _T("EN"), _T("Alphadream Corporation")			  },
	{ _T("F0"), _T("A Wave")							  },
	{ _T("G1"), _T("PCCW")								  },
	{ _T("G4"), _T("KiKi Co Ltd")						  },
	{ _T("G5"), _T("Open Sesame Inc.")					  },
	{ _T("G6"), _T("Sims")								  },
	{ _T("G7"), _T("Broccoli")							  },
	{ _T("G8"), _T("Avex")								  },
	{ _T("G9"), _T("D3 Publisher")						  },
	{ _T("GB"), _T("Konami Computer Entertainment Japan") },
	{ _T("GD"), _T("Square-Enix")						  },
	{ _T("HY"), _T("Sachen")							  },
	{ NULL, NULL								  }
};

// The cartridge header holds byte fields, not strings in the machine's code
// page: they are copied through the ANSI code page, which is how these bytes
// have always been read.
static void winRomInfoCopyHeader(TCHAR *out, int outSize, const u8 *bytes, int count)
{
	char text[64];

	if (count > (int)sizeof(text) - 1)
		count = (int)sizeof(text) - 1;

	memcpy(text, bytes, (size_t)count);
	text[count] = 0;

	const CString wide = winWideFromAnsi(text);
	lstrcpyn(out, wide, outSize);
}

static LPCTSTR winGBARomInfoFindMakerCode(LPCTSTR code)
{
	int i = 0;
	while (winGBARomInfoCompanies[i].code)
	{
		if (!_tcscmp(winGBARomInfoCompanies[i].code, code))
			return winGBARomInfoCompanies[i].name;
		i++;
	}
	return (LPCTSTR)winResLoadString(IDS_UNKNOWN);
}

/////////////////////////////////////////////////////////////////////////////
// RomInfoGB dialog

RomInfoGB::RomInfoGB(u8 *rom, CWnd *pParent /*=NULL*/)
	: CDialog(RomInfoGB::IDD, pParent)
{
	//{{AFX_DATA_INIT(RomInfoGB)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	this->rom = rom;
}

void RomInfoGB::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RomInfoGB)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(RomInfoGB, CDialog)
//{{AFX_MSG_MAP(RomInfoGB)
ON_BN_CLICKED(ID_OK, OnOk)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RomInfoGB message handlers

void RomInfoGB::OnOk()
{
	EndDialog(TRUE);
}

BOOL RomInfoGB::OnInitDialog()
{
	CDialog::OnInitDialog();

	TCHAR buffer[128];

	winRomInfoCopyHeader(buffer, _countof(buffer), &rom[0x134], 15);
	GetDlgItem(IDC_ROM_TITLE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0x143]);
	GetDlgItem(IDC_ROM_COLOR)->SetWindowText(buffer);

	winRomInfoCopyHeader(buffer, _countof(buffer), &rom[0x144], 2);
	GetDlgItem(IDC_ROM_MAKER_CODE)->SetWindowText(buffer);

	if (rom[0x14b] != 0x33)
	{
		_stprintf(buffer, _T("%02X"), rom[0x14b]);
		GetDlgItem(IDC_ROM_MAKER_CODE)->SetWindowText(buffer);
	}
	GetDlgItem(IDC_ROM_MAKER_NAME2)->SetWindowText(winGBARomInfoFindMakerCode(buffer));

	_stprintf(buffer, _T("%02x"), rom[0x146]);
	GetDlgItem(IDC_ROM_UNIT_CODE)->SetWindowText(buffer);

	CString type = winResLoadString(IDS_UNKNOWN);
	switch (rom[0x147])
	{
	case 0x00:
		type = "ROM";
		break;
	case 0x01:
		type = "ROM+MBC1";
		break;
	case 0x02:
		type = "ROM+MBC1+RAM";
		break;
	case 0x03:
		type = "ROM+MBC1+RAM+BATT";
		break;
	case 0x05:
		type = "ROM+MBC2";
		break;
	case 0x06:
		type = "ROM+MBC2+BATT";
		break;
	case 0x0f:
		type = "ROM+MBC3+TIMER+BATT";
		break;
	case 0x10:
		type = "ROM+MBC3+TIMER+RAM+BATT";
		break;
	case 0x11:
		type = "ROM+MBC3";
		break;
	case 0x12:
		type = "ROM+MBC3+RAM";
		break;
	case 0x13:
		type = "ROM+MBC3+RAM+BATT";
		break;
	case 0x19:
		type = "ROM+MBC5";
		break;
	case 0x1a:
		type = "ROM+MBC5+RAM";
		break;
	case 0x1b:
		type = "ROM+MBC5+RAM+BATT";
		break;
	case 0x1c:
		type = "ROM+MBC5+RUMBLE";
		break;
	case 0x1d:
		type = "ROM+MBC5+RUMBLE+RAM";
		break;
	case 0x1e:
		type = "ROM+MBC5+RUMBLE+RAM+BATT";
		break;
	case 0x22:
		type = "ROM+MBC7+BATT";
		break;
	case 0xfe:
		type = "ROM+HuC-3";
		break;
	case 0xff:
		type = "ROM+HuC-1";
		break;
	}
	_stprintf(buffer, _T("%02x (%s)"), rom[0x147], (LPCTSTR)type);
	GetDlgItem(IDC_ROM_DEVICE_TYPE)->SetWindowText(buffer);

	type = winResLoadString(IDS_UNKNOWN);
	switch (rom[0x148])
	{
	case 0:
		type = "32K";
		break;
	case 1:
		type = "64K";
		break;
	case 2:
		type = "128K";
		break;
	case 3:
		type = "256K";
		break;
	case 4:
		type = "512K";
		break;
	case 5:
		type = "1M";
		break;
	case 6:
		type = "2M";
		break;
	case 7:
		type = "4M";
		break;
	}

	_stprintf(buffer, _T("%02x (%s)"), rom[0x148], (LPCTSTR)type);
	GetDlgItem(IDC_ROM_SIZE)->SetWindowText(buffer);

	type = winResLoadString(IDS_UNKNOWN);
	switch (rom[0x149])
	{
	case 0:
		type = winResLoadString(IDS_NONE);
		break;
	case 1:
		type = "2K";
		break;
	case 2:
		type = "8K";
		break;
	case 3:
		type = "32K";
		break;
	case 4:
		type = "128K";
		break;
	case 5:
		type = "64K";
		break;
	}

	_stprintf(buffer, _T("%02x (%s)"), rom[0x149], (LPCTSTR)type);
	GetDlgItem(IDC_ROM_RAM_SIZE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0x14a]);
	GetDlgItem(IDC_ROM_DEST_CODE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0x14b]);
	GetDlgItem(IDC_ROM_LIC_CODE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0x14c]);
	GetDlgItem(IDC_ROM_VERSION)->SetWindowText(buffer);

	u8	crc8 = 25;
	int i;
	for (i = 0x134; i < 0x14d; i++)
	{
		crc8 = (crc8 + rom[i]) & 0xFF;
	}

	crc8 = 256 - crc8;

	_stprintf(buffer, _T("%02x (%02x)"), crc8, rom[0x14d]);
	GetDlgItem(IDC_ROM_CRC)->SetWindowText(buffer);

	u16 crc16 = 0;
	for (i = 0; i < gbRomSize; i++)
	{
		crc16 = (crc16 + rom[i]) & 0xFFFF;
	}

	crc16 -= rom[0x14e];
	crc16 -= rom[0x14f];
	_stprintf(buffer, _T("%04x (%04x)"), crc16, (rom[0x14e] << 8) | rom[0x14f]);
	GetDlgItem(IDC_ROM_CHECKSUM)->SetWindowText(buffer);

	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// RomInfoGBA dialog

RomInfoGBA::RomInfoGBA(u8 *rom, CWnd *pParent /*=NULL*/)
	: CDialog(RomInfoGBA::IDD, pParent)
{
	//{{AFX_DATA_INIT(RomInfoGBA)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	this->rom = rom;
}

void RomInfoGBA::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RomInfoGBA)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(RomInfoGBA, CDialog)
//{{AFX_MSG_MAP(RomInfoGBA)
ON_BN_CLICKED(ID_OK, OnOk)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RomInfoGBA message handlers

void RomInfoGBA::OnOk()
{
	EndDialog(TRUE);
}

BOOL RomInfoGBA::OnInitDialog()
{
	CDialog::OnInitDialog();

	TCHAR buffer[13];

	winRomInfoCopyHeader(buffer, _countof(buffer), &rom[0xa0], 12);
	GetDlgItem(IDC_ROM_TITLE)->SetWindowText(buffer);

	winRomInfoCopyHeader(buffer, _countof(buffer), &rom[0xac], 4);
	GetDlgItem(IDC_ROM_GAME_CODE)->SetWindowText(buffer);

	winRomInfoCopyHeader(buffer, _countof(buffer), &rom[0xb0], 2);
	GetDlgItem(IDC_ROM_MAKER_CODE)->SetWindowText(buffer);

	GetDlgItem(IDC_ROM_MAKER_NAME)->SetWindowText(winGBARomInfoFindMakerCode(buffer));

	_stprintf(buffer, _T("%02x"), rom[0xb3]);
	GetDlgItem(IDC_ROM_UNIT_CODE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0xb4]);
	GetDlgItem(IDC_ROM_DEVICE_TYPE)->SetWindowText(buffer);

	_stprintf(buffer, _T("%02x"), rom[0xbc]);
	GetDlgItem(IDC_ROM_VERSION)->SetWindowText(buffer);

	u8 crc = 0x19;
	for (int i = 0xa0; i < 0xbd; i++)
	{
		crc = (crc + rom[i]) & 0xff;
	}

	crc = (-crc) & 0xff;

	_stprintf(buffer, _T("%02x (%02x)"), crc, rom[0xbd]);
	GetDlgItem(IDC_ROM_CRC)->SetWindowText(buffer);
	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

