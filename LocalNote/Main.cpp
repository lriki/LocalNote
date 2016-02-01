/*
pandoc -D html5 > html5_template.html

pandoc -f markdown -t html5 -o index.html index.md

*/
#include "stdafx.h"
#include "Manager.h"

int main(int argc, char **argv)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	setlocale(LC_ALL, "");

	try
	{
		Manager manager;
		manager.Execute(_T("../docs/src"), _T("../docs/templates"), _T("../docs/release"));
	}
	catch (Exception& e)
	{
		_tprintf(e.GetMessage());
		return 1;
	}
	return 0;
}

#if 0

class Manager
{
public:

	struct LinkElement
	{
		String	href;
		String	text;
	};

	struct PageInfo
	{
		PathName	srcPath;
		PathName	srcFullPath;
		String		caption;

		PathName	outputRelPath;
		PathName	outputFullPath;
		PathName	relpathToRoot;

		String		htmlText;
		Array<LinkElement>	m_linkElementList;

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

		void Expand()
		{
			String text;
			String args = String::Format(_T("-f markdown -t html5 -o tmp \"{0}\""), srcFullPath);
			Process::Execute(_T("pandoc"), args);

			StreamReader reader(_T("tmp"), Encoding::GetUTF8Encoding());
			StringWriter writer;
			String line;
			while (reader.ReadLine(&line))
			{
				MatchResult m;
				if (Regex::Match(line, _T("<h2 id=\"(.*)\">(.*)</h2>"), &m))
				{
					m_linkElementList.Add(LinkElement{m[1], m[2]});
				}
				
				writer.WriteLine(line);
			}
			htmlText = writer.ToString();
		}

		String MakePageIndexList() const
		{
			// http://am-yu.net/2014/04/20/bootstrap3-affix-scrollspy/
			StringWriter writer;
			writer.WriteLine(_T(R"(<nav class="affix-nav"><ul class="nav">)"));
			writer.WriteLine(_T(R"(<li class="cap">page contents</li>)"));
			for (auto& e : m_linkElementList)
			{
				writer.WriteLine(String::Format(_T(R"(<li><a href="#{0}">{1}</a></li>)"), e.href, e.text));
			}
			writer.WriteLine(_T(R"(</ul></nav>)"));
			return writer.ToString();
#if 0
			StringWriter writer;
			writer.WriteLine(_T(R"(<nav class="affix-nav"><div class="list-group nav">)"));
			for (auto& e : m_linkElementList)
			{
				writer.WriteLine(String::Format(_T(R"(<a href="#{0}" class="list-group-item">{1}</a>)"), e.href, e.text));
			}
			writer.WriteLine(_T(R"(</div></nav>)"));
			return writer.ToString();
#endif
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
				reader.GetName() == _T("CategoryItem") &&
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
						item.info.Expand();
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

	// page についての html ファイルを作る
	void MakePageFile(const NavbarItem* active, const PageInfo& page)
	{
		String pageText = FileSystem::ReadAllText(PathName(m_pathTemplate, _T("page.html")).c_str(), Encoding::GetUTF8Encoding());
		pageText = pageText.Replace(_T("NAVBAR_ITEMS"), MakeNavbatText(active, page).c_str());
		pageText = pageText.Replace(_T("PAGE_CONTENTS"), page.htmlText.c_str());
		pageText = pageText.Replace(_T("PAGE_INDEX_LIST"), page.MakePageIndexList().c_str());

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
#endif

