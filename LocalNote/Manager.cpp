#include "stdafx.h"
#include "Manager.h"

//=============================================================================
// Page
//=============================================================================

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
Page::Page(Manager* manager, CategoryItem* ownerCategory, const PathName& srcFileRelPath)
{
	m_manager = manager;
	m_ownerCategory = ownerCategory;
	m_srcFileRelPath = srcFileRelPath;
	m_srcFileFullPath = PathName(manager->m_srcRootDir, m_srcFileRelPath);
	m_outputFileRelPath = m_srcFileRelPath.ChangeExtension(_T("html"));
	m_outputFileFullPath = PathName(manager->m_outputRootDir, m_outputFileRelPath);
	m_relpathToRoot = m_outputFileFullPath.GetParent().MakeRelative(manager->m_outputRootDir);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Page::BuildContents()
{
	String text;
	String args = String::Format(_T("-f markdown -t html5 -o tmp \"{0}\""), m_srcFileFullPath);
	Process::Execute(_T("pandoc"), args);	// TODO: 標準出力のエンコーディングを指定したい・・・。とりあえず今は一度ファイルに落とす

	// 一時ファイルを開きなおして解析する
	StreamReader reader(_T("tmp"), Encoding::GetUTF8Encoding());
	StringWriter writer;
	String line;
	while (reader.ReadLine(&line))
	{
		// <h1> はキャプション
		MatchResult m1;
		if (Regex::Match(line, _T("<h1 id=\"(.*)\">(.*)</h1>"), &m1))
		{
			m_caption = m1[2];
		}
		// <h2> を集めておく
		MatchResult m;
		if (Regex::Match(line, _T("<h2 id=\"(.*)\">(.*)</h2>"), &m))
		{
			m_linkElementList.Add(LinkElement{ m[1], m[2] });
		}

		writer.WriteLine(line);
	}
	m_pageContentsText = writer.ToString();

	// ナビゲーション
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
		m_pageNavListText = writer.ToString();
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
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Page::ExportPageFile()
{
	String pageText = FileSystem::ReadAllText(PathName(m_manager->m_templateDir, _T("page.html")).c_str(), Encoding::GetUTF8Encoding());
	pageText = pageText.Replace(_T("NAVBAR_ITEMS"), m_ownerCategory->MakeNavbarTextInActive(this));
	pageText = pageText.Replace(_T("PAGE_CONTENTS"), m_pageContentsText);
	pageText = pageText.Replace(_T("PAGE_INDEX_LIST"), m_pageNavListText);

	FileSystem::CreateDirectory(m_outputFileFullPath.GetParent());
	FileSystem::WriteAllText(m_outputFileFullPath.c_str(), pageText, Encoding::GetUTF8Encoding());
}

//=============================================================================
// TocTreeItem
//=============================================================================

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RefPtr<TocTreeItem> TocTreeItem::Deserialize(XmlFileReader* reader, CategoryToc* ownerToc)
{
	auto item = RefPtr<TocTreeItem>::MakeRef();
	ownerToc->m_allTreeItemList.Add(item);

	if (reader->MoveToFirstAttribute())
	{
		do
		{
			if (reader->GetName() == _T("page"))
			{
				auto page = RefPtr<Page>::MakeRef(ownerToc->m_manager, ownerToc->m_ownerCategoryItem, reader->GetValue());
				ownerToc->m_manager->m_allPageList.Add(page);
				item->m_page = page;
			}
			if (reader->GetName() == _T("caption"))
			{
				item->m_caption = reader->GetValue();
			}

		} while (reader->MoveToNextAttribute());

		reader->MoveToElement();
	}

	if (reader->IsEmptyElement()) return item;

	while (reader->Read())
	{
		if (reader->GetNodeType() == XmlNodeType::EndElement) return item;

		if (reader->GetNodeType() == XmlNodeType::Element &&
			reader->GetName() == _T("TreeItem"))
		{
			item->m_children.Add(Deserialize(reader, ownerToc));
		}
	}

	return item;
}

//=============================================================================
// CategoryToc
//=============================================================================

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RefPtr<CategoryToc> CategoryToc::Deserialize(XmlFileReader* reader, CategoryItem* owner)
{
	auto item = RefPtr<CategoryToc>::MakeRef();
	item->m_manager = owner->m_manager;
	item->m_ownerCategoryItem = owner;

	// attr は今のところ無い

	if (reader->IsEmptyElement()) return item;

	while (reader->Read())
	{
		if (reader->GetNodeType() == XmlNodeType::EndElement) return item;

		if (reader->GetNodeType() == XmlNodeType::Element &&
			reader->GetName() == _T("TreeItem"))
		{
			item->m_rootTreeItemList.Add(TocTreeItem::Deserialize(reader, item));
		}
	}

	return item;
}

//=============================================================================
// CategoryItem
//=============================================================================

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RefPtr<CategoryItem> CategoryItem::Deserialize(XmlFileReader* reader, Manager* manager, CategoryItem* parent)
{
	auto item = RefPtr<CategoryItem>::MakeRef();
	item->m_manager = manager;
	manager->m_allCategoryItemList.Add(item);

	if (reader->MoveToFirstAttribute())
	{
		do
		{
			if (reader->GetName() == _T("page"))
			{
				auto page = RefPtr<Page>::MakeRef(manager, item, reader->GetValue());
				manager->m_allPageList.Add(page);
				item->m_page = page;
			}
			else if (reader->GetName() == _T("caption"))
			{
				item->m_caption = reader->GetValue();
			}
			else if (reader->GetName() == _T("tok"))
			{
				item->m_srcTokPath = PathName(manager->m_srcRootDir, reader->GetValue());
			}

		} while (reader->MoveToNextAttribute());

		reader->MoveToElement();
	}

	if (reader->IsEmptyElement()) return item;

	while (reader->Read())
	{
		if (reader->GetNodeType() == XmlNodeType::EndElement) return item;

		if (reader->GetNodeType() == XmlNodeType::Element &&
			reader->GetName() == _T("CategoryItem"))
		{
			item->m_children.Add(Deserialize(reader, manager, item));
		}
	}

	return item;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CategoryItem::MakeToc()
{
	if (!m_srcTokPath.IsEmpty())
	{
		XmlFileReader reader(PathName(m_manager->m_srcRootDir, m_srcTokPath));
		while (reader.Read())
		{
			if (reader.GetNodeType() == XmlNodeType::Element &&
				reader.GetName() == _T("Toc"))
			{
				m_toc = CategoryToc::Deserialize(&reader, this);
			}
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
String CategoryItem::MakeNavbarTextInActive(Page* page)
{
	StringWriter writer;
	for (auto& item : m_manager->m_rootCategoryItemList)
	{
		// <a>～</a> を作る
		String linkSpan;
		if (item->m_page != nullptr)
		{
			String relPath = String::Format(_T("{0}/{1}"), page->GetRelPathToRoot(), item->m_page->GetOutputRelPath());	// root に戻って、指定のページに移動するようなパス
			linkSpan = String::Format(_T("<a href=\"{0}\">{1}</a>"), relPath.Replace(_T("\\"), _T("/")), item->m_page->GetCaption());
		}
		else
		{
			// リンクを持たないアイテムもある
			linkSpan = String::Format(_T("<a>{0}</a>"), item->m_caption);
		}

		// 子要素の有無によって <li> を工夫する
		if (item->m_children.IsEmpty())
		{
			writer.WriteLine(String::Format(_T("<li>{0}</li>"), linkSpan));
		}
		else
		{
			// 子要素を持っているのでドロップダウンにする
			writer.WriteLine(_T("<li class=\"dropdown\">"));
			linkSpan = String::Format(_T(R"(<a href="#" class="dropdown-toggle" data-toggle="dropdown">{0}<b class="caret"></b></a>)"), item->m_caption);
			writer.WriteLine(linkSpan);
			writer.WriteLine(_T("<ul class=\"dropdown-menu\">"));

			for (auto& child : item->m_children)
			{
				String relPath = String::Format(_T("{0}/{1}"), page->GetRelPathToRoot(), child->m_page->GetOutputRelPath());	// root に戻って、指定のページに移動するようなパス
				linkSpan = String::Format(_T("<a href=\"{0}\">{1}</a>"), relPath.Replace(_T("\\"), _T("/")), child->m_page->GetCaption());

				writer.WriteLine(String::Format(_T("<li>{0}</li>"), linkSpan));
			}

			writer.WriteLine(_T("</ul>"));
			writer.WriteLine(_T("</li>"));
		}
	}
	return writer.ToString();
}

//=============================================================================
// Manager
//=============================================================================

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Manager::Execute(const PathName& srcDir, const PathName& templateDir, const PathName& releaseDir)
{
	m_srcRootDir = srcDir.CanonicalizePath();
	m_templateDir = templateDir.CanonicalizePath();
	m_outputRootDir = releaseDir.CanonicalizePath();

	PathName srcIndexPath(srcDir, _T("index.xml"));
	XmlFileReader reader(srcIndexPath);
	while (reader.Read())
	{
		if (reader.GetNodeType() == XmlNodeType::Element &&
			reader.GetName() == _T("CategoryItem"))
		{
			auto item = CategoryItem::Deserialize(&reader, this, nullptr);
			m_rootCategoryItemList.Add(item);
		}
	}

	// TOC が必要なカテゴリは作る
	for (auto& item : m_allCategoryItemList)
	{
		item->MakeToc();
	}

	// ページ内容を作る
	for (auto& page : m_allPageList)
	{
		page->BuildContents();
		page->ExportPageFile();
	}

	printf("");
}

