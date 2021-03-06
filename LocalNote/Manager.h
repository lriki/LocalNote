#pragma once

class Manager;
class CategoryItem;
class CategoryToc;

class Page : public RefObject
{
public:
	Page(Manager* manager, CategoryItem* ownerCategory, const PathName& dummy, const PathName& srcFileFullPath);

	const PathName& GetOutputRelPath() const { return m_outputFileRelPath; }
	const PathName& GetRelPathToRoot() const { return m_relpathToRoot; }
	const String& GetCaption() const { return m_caption; }

	// このページから指定したページへ飛ぶための相対パスを作成する
	PathName MakeRelativePath(Page* page) const;

	void BuildContents();

	void ExportPageFile();

public:
	struct LinkElement
	{
		String	href;
		String	text;
	};

	Manager*		m_manager;
	CategoryItem*	m_ownerCategory;

	PathName		m_srcFileRelPath;
	PathName		m_srcFileFullPath;	// .md のパス
	PathName		m_outputFileRelPath;
	PathName		m_outputFileFullPath;
	PathName		m_relpathToRoot;

	String				m_caption;
	String				m_pageContentsText;
	String				m_pageNavListText;
	Array<LinkElement>	m_linkElementList;
};

class TocTreeItem : public RefObject
{
public:
	// Deserialize で設定
	PathName					m_srcPagePath;		// .md のパス
	String						m_caption;
	Array<TocTreeItem*>			m_children;
	Page*						m_page;

	static RefPtr<TocTreeItem> Deserialize(XmlFileReader* reader, CategoryToc* ownerToc);

	String GetCaption() const;
};

class CategoryToc : public RefObject
{
public:
	Manager*					m_manager;
	CategoryItem*				m_ownerCategoryItem;

	// Deserialize で設定
	PathName					m_srcPageFullPath;
	Array<TocTreeItem*>			m_rootTreeItemList;
	Array<RefPtr<TocTreeItem>>	m_allTreeItemList;

	static RefPtr<CategoryToc> Deserialize(XmlFileReader* reader, PathName xmlFileParentFullPath, CategoryItem* owner);

	Page* GetRootPage() { return m_rootPage; }

	// page : ナビバーを埋め込みたいページ
	String MakeTocTree(Page* page);

private:

	Page*	m_rootPage = nullptr;
};

class CategoryItem : public RefObject
{
public:
	Manager*					m_manager;

	// Deserialize で設定
	PathName					m_srcPagePath;		// .md のパス
	String						m_caption;
	PathName					m_srcTokRelPath;
	PathName					m_srcTokFullPath;		// .xml のパス
	Array<RefPtr<CategoryItem>>	m_children;

	Page*						m_page;
	RefPtr<CategoryToc>			m_toc;

	static RefPtr<CategoryItem> Deserialize(XmlFileReader* reader, Manager* manager, CategoryItem* parent);

	Page* GetCategoryRootPage();
	void MakeToc();

	// page : ナビバーを埋め込みたいページ
	String MakeNavbarTextInActive(Page* page);
};

class Manager
{
public:
	void Execute(const PathName& srcDir, const PathName& templateDir, const PathName& releaseDir);

public:

	PathName	m_srcRootDir;
	PathName	m_templateDir;
	PathName	m_outputRootDir;

	Array<CategoryItem*>		m_rootCategoryItemList;
	Array<RefPtr<CategoryItem>>	m_allCategoryItemList;
	Array<RefPtr<Page>>			m_allPageList;
};
