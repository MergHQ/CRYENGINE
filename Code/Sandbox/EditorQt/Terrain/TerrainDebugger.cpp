#include "StdAfx.h"
#include "Terrain/TerrainManager.h"

#include "IEditorImpl.h"

#include <PathUtils.h>
#include <Util/ImageUtil.h>
#include <QtUtil.h>
#include <QFileInfo>
#include <QDir>

namespace Private_TerrainDumper
{

QString CreateRootDumpFolder()
{
	QString rootOutputFolder = QtUtil::ToQString(PathUtil::GetUserSandboxFolder()) + "Dump/Terrain/";

	//Create unique subfolder: don't delete old one: that will be used for diffs
	for (unsigned i = 0; i < 1000; ++i)
	{
		QString outputFolder = rootOutputFolder + std::to_string(i).c_str() + '/';
		QFileInfo inf(outputFolder);
		if (inf.exists() && inf.isDir())
		{
			continue;
		}

		QDir dir(outputFolder);
		dir.mkpath(outputFolder);

		return outputFolder;
	}

	//Too many folders: delete them all, start from 0
	QDir dir(rootOutputFolder);
	dir.removeRecursively();

	rootOutputFolder += "0/";
	dir.mkpath(rootOutputFolder);
	return rootOutputFolder;
}

void DumpRgbTiles(const QString& rootFolder)
{
	const QString tileFolder = rootFolder + "RgbTiles";
	QDir dir(tileFolder);
	dir.mkdir(tileFolder);

	GetIEditorImpl()->GetTerrainManager()->GetHeightmap()->GetRGBLayer()->Debug(QtUtil::ToString(tileFolder));
}

void DumpDetailedLayers(const QString& rootFolder)
{
	const QString tileFolder = rootFolder + "DetailedLayers";
	QDir dir(tileFolder);
	dir.mkdir(tileFolder);

	const CSurfTypeImage& img = GetIEditorImpl()->GetTerrainManager()->GetHeightmap()->GetLayersImage();

	const int cx = img.GetWidth();
	const int cy = img.GetHeight();

	CImageEx layers;
	layers.Allocate(cx, cy);

	CImageEx weights1, weights2, weights3;
	weights1.Allocate(cx, cy);
	weights2.Allocate(cx, cy);
	weights3.Allocate(cx, cy);

	CImageEx holes;
	holes.Allocate(cx, cy);

	const SSurfaceTypeItem* pData = img.GetData();

	for (int y = 0; y < cy; ++y)
	{
		for (int x = 0; x < cx; ++x)
		{
			const SSurfaceTypeItem& pixel = *(pData + x + cy * y);
			layers.ValueAt(x, y) = (pixel.ty[0] << 24) + (pixel.ty[1] << 16) + (pixel.ty[2] << 8);
			weights1.ValueAt(x, y) = (pixel.we[0] << 16); //Write all 3 weights in separate files, blue channel
			weights2.ValueAt(x, y) = (pixel.we[1] << 16);
			weights3.ValueAt(x, y) = (pixel.we[2] << 16);
			holes.ValueAt(x, y) = pixel.hole ? 0 : 127;
		}
	}

	const string outFolder = QtUtil::ToString(tileFolder);

	CImageUtil::SaveBitmap(PathUtil::Make(outFolder, "layers.bmp"), layers);
	CImageUtil::SaveBitmap(PathUtil::Make(outFolder, "weights1.bmp"), weights1);
	CImageUtil::SaveBitmap(PathUtil::Make(outFolder, "weights2.bmp"), weights2);
	CImageUtil::SaveBitmap(PathUtil::Make(outFolder, "weights3.bmp"), weights3);
	CImageUtil::SaveBitmap(PathUtil::Make(outFolder, "holes.bmp"), holes);
}

void DumpHightmap(const QString& rootFolder)
{
	const string path = PathUtil::Make(QtUtil::ToString(rootFolder), "hightmap.bmp");
	GetIEditorImpl()->GetTerrainManager()->GetHeightmap()->SaveImage(path.c_str());
}

void DumpTerrainData()
{
	const QString rootFolder = CreateRootDumpFolder();
	DumpRgbTiles(rootFolder);
	DumpDetailedLayers(rootFolder);
	DumpHightmap(rootFolder);
}

} // namespace Private_TerrainDumper

//REGISTER_EDITOR_COMMAND(Private_TerrainDumper::DumpTerrainData, editor, dump_terrain_resources, CCommandDescription("Dump terrain layers to <project folder>/user/Dump/Terrain/"));
