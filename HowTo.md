# Getting started #
  1. Extract the archive you downloaded to your SD-Card. The archive represents the root of the SD-Card.
  1. Create a "feeds.xml" in the root of your SD-Card.

# The "feeds.xml" #
Here you see a sample-"feeds-xml"-file:
```
<?xml version="1.0" encoding="utf-8"?>
<rss>
  <feed name="Example-Feed"><![CDATA[http://example.com/rss-feed]]></feed>
  <feed name="One more Example-Feed!"><![CDATA[http://example.com/another_rss-feed]]></feed>
</rss>
```
You can add as many feed-nodes as you want. The "name"-Attribute will be displayed on top of the message at the Wii-Messageboard, so it is recommended to cut that REALLY SHORT because there's hardly any space. Note: Don't forget the CDATA-Tags!

# Launching RSSMii #
  1. Launch RSSMii in the Homebrew-Channel or any other launcher of your choice (I thought of custom channels, so launching this won't result in a CodeDump-Error on exit!)
  1. You see the RSS-Feeds listed in the "feeds.xml"
  1. When you now press A, every previous subscriptions to RSS-Feeds will be overwritten by subscriptions to the Feeds listed in the "feeds.xml".
  1. Everything's finished - You should now be receiving updates at those RSS-Feeds at the Wii-Messageboard!