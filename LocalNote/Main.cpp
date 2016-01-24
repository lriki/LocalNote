/*
pandoc -D html5 > html5_template.html

pandoc -f markdown -t html5 -o index.html index.md

*/
#include "stdafx.h"
using namespace ln;

class Manager
{
public:

	struct PageInfo
	{
		PathName	srcPath;
		PathName	srcFullPath;
		String		caption;

		PathName	outputRelPath;
		PathName	outputFullPath;
		PathName	relpathToRoot;

		void AnalyzeMarkdown()
		{
			StreamReader reader(srcFullPath);
			String line;
			String lastLine;
			while (reader.ReadLine(&line))
			{
				if (line.Contains(_T("====")))
				{
					caption = lastLine;
					break;
				}
				lastLine = line;
			}
		}

		String MakeContents() const
		{
			String text;
			String args = String::Format(_T("-f markdown -t html5 -o tmp \"{0}\""), srcFullPath);
			Process::Execute(_T("pandoc"), args);
			return FileSystem::ReadAllText(_T("tmp"), Encoding::GetUTF8Encoding());
		}
	};

	struct NavbarItem
	{
		PageInfo info;
	};

	void Analyze()
	{
		m_pathOutput = m_pathOutput.CanonicalizePath();

		CollectSourceFiles();

		for (auto& item : m_navbarItemList)
		{
			MakePageFile(&item, item.info);
		}
	}

private:

	void CollectSourceFiles()
	{
		PathName srcRootPath = _T("../docs/src");
		PathName srcIndexPath(srcRootPath, _T("index.xml"));

		XmlFileReader reader(srcIndexPath);
		while (reader.Read())
		{
			if (reader.GetNodeType() == XmlNodeType::Element &&
				reader.GetName() == _T("NavbarItem") &&
				reader.MoveToFirstAttribute())
			{
				do
				{
					if (reader.GetName() == _T("src"))
					{
						NavbarItem item;
						item.info.srcPath = reader.GetValue();
						item.info.srcFullPath = PathName(srcRootPath, reader.GetValue());
						item.info.outputRelPath = item.info.srcPath.ChangeExtension(_T("html"));
						item.info.outputFullPath = PathName(m_pathOutput, item.info.outputRelPath);
						item.info.relpathToRoot = item.info.outputFullPath.GetParent().MakeRelative(m_pathOutput);
						item.info.AnalyzeMarkdown();
						m_navbarItemList.Add(item);
					}

				} while (reader.MoveToNextAttribute());
			}
		}
	}

	// page : ナビバーを埋め込みたいページ
	String MakeNavbatText(const NavbarItem* active, const PageInfo& page)
	{
		StringWriter writer;
		for (auto& item : m_navbarItemList)
		{
			String relPath = String::Format(_T("{0}/{1}"), page.relpathToRoot, item.info.outputRelPath);
			writer.WriteLine(String::Format(_T("<li><a href=\"{0}\">{1}</a></li>"), relPath.Replace(_T("\\"), _T("/")), item.info.caption));
		}
		return writer.ToString();
	}

	void MakePageFile(const NavbarItem* active, const PageInfo& page)
	{
		String pageText = FileSystem::ReadAllText(PathName(m_pathTemplate, _T("page.html")).c_str(), Encoding::GetUTF8Encoding());
		pageText = pageText.Replace(_T("NAVBAR_ITEMS"), MakeNavbatText(active, page).c_str());
		pageText = pageText.Replace(_T("PAGE_CONTENTS"), page.MakeContents().c_str());

		FileSystem::CreateDirectory(page.outputFullPath.GetParent());
		FileSystem::WriteAllText(page.outputFullPath.c_str(), pageText, Encoding::GetUTF8Encoding());
	}

private:
	PathName			m_pathOutput = _T("../docs/release");
	PathName			m_pathTemplate = _T("../docs/templates");
	Array<NavbarItem>	m_navbarItemList;
};

int main()
{
	try
	{
		Manager manager;
		manager.Analyze();
	}
	catch (Exception& e)
	{
		_tprintf(e.GetMessage());
		return 1;
	}
    return 0;
}

