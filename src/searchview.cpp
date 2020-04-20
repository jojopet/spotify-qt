#include "searchview.hpp"

SearchView::SearchView(spt::Spotify &spotify, QWidget *parent) : QDockWidget(parent)
{
	auto window = (MainWindow*) parent;
	auto layout = new QVBoxLayout();
	layout->setContentsMargins(-1, 0, -1, 0);
	auto searchBox = new QLineEdit(this);
	layout->addWidget(searchBox);
	// Tabs
	auto tabs = new QTabWidget(this);
	layout->addWidget(tabs);
	// All lists
	artistList		= new QListWidget(this);
	playlistList	= new QListWidget(this);
	// Track list
	trackList = defaultTree({
		"Title", "Artist"
	});
	// Album list
	albumList = defaultTree({
		"Title", "Artist"
	});
	// Add all tabs
	tabs->addTab(trackList,		"Tracks");
	tabs->addTab(artistList,	"Artists");
	tabs->addTab(albumList,		"Albums");
	tabs->addTab(playlistList,	"Playlists");

	// Start searching when pressing enter
	QLineEdit::connect(searchBox, &QLineEdit::returnPressed, [this, &spotify, searchBox, window]() {
		// Empty all previous results
		trackList->clear();
		artistList->clear();
		albumList->clear();
		playlistList->clear();
		// Get new results
		searchBox->setEnabled(false);
		auto results = spotify.search(searchBox->text());
		// Albums
		for (auto &album : results.albums)
		{
			auto item = new QTreeWidgetItem({
				album.name, album.artist
			});
			item->setIcon(0, window->getAlbum(album.image));
			item->setData(0, MainWindow::RoleAlbumId, album.id);
			albumList->addTopLevelItem(item);
		}
		// Artists
		for (auto &artist : results.artists)
		{
			auto item = new QListWidgetItem(artist.name, artistList);
			item->setData(MainWindow::RoleArtistId, artist.id);
		}
		// Playlists
		for (auto &playlist : results.playlists)
		{
			auto item = new QListWidgetItem(spt::Playlist(playlist).name, playlistList);
			item->setData(0x100, playlist);
		}
		// Tracks
		for (auto &track : results.tracks)
		{
			auto item = new QTreeWidgetItem(trackList,{
				track.name, track.artist
			});
			item->setData(0, MainWindow::RoleTrackId, track.id);
		}
		// Search done
		searchBox->setEnabled(true);
	});

	// Open album
	QTreeWidget::connect(albumList, &QTreeWidget::itemClicked, [this, window](QTreeWidgetItem *item, int column) {
		if (!window->loadAlbum(item->data(0, MainWindow::RoleAlbumId).toString(), false))
			window->setStatus(QString("Failed to load album"));
	});
	// Open artist
	QListWidget::connect(artistList, &QListWidget::itemClicked, [this, window](QListWidgetItem *item) {
		window->openArtist(item->data(MainWindow::RoleArtistId).toString());
		close();
	});
	// Open playlist
	QListWidget::connect(playlistList, &QListWidget::itemClicked, [this, window](QListWidgetItem *item) {
		auto playlist = spt::Playlist(item->data(0x100).value<QJsonObject>());
		if (!window->loadPlaylist(playlist))
			window->setStatus(QString("Failed to load playlist"));
		else
			window->playlists->setCurrentRow(-1);
	});
	// Open track
	QTreeWidget::connect(trackList, &QTreeWidget::itemClicked, [this, window, &spotify](QTreeWidgetItem *item, int column) {
		// Do we want it to continue playing results?
		auto trackId = QString("spotify:track:%1")
			.arg(item->data(0, MainWindow::RoleTrackId).toString());
		auto status = spotify.playTracks(trackId, QStringList(trackId));
		if (!status.isEmpty())
			window->setStatus(QString("Failed to play track: %1").arg(status));
	});

	// Setup dock
	setWindowTitle("Search");
	setWidget(MainWindow::layoutToWidget(layout));
	setFixedWidth(320);
	// Uncheck search when closing
	QDockWidget::connect(this, &QDockWidget::visibilityChanged, [window](bool visible) {
		window->search->setChecked(visible);
	});
}

QTreeWidget *SearchView::defaultTree(const QStringList &headers)
{
	auto tree = new QTreeWidget(this);
	tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree->setSelectionBehavior(QAbstractItemView::SelectRows);
	tree->setRootIsDecorated(false);
	tree->setAllColumnsShowFocus(true);
	tree->setColumnCount(headers.length());
	tree->setHeaderLabels(headers);
	return tree;
}