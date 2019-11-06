const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin')
const VersionFile = require('webpack-version-file');

module.exports = {
  entry: './web_src/index.js',
  output: {
    filename: 'main.js',
    path: path.resolve(__dirname, 'data'),
  },
  module: {
    rules: [
      {
        test: /\.css$/,
        use: [
          'style-loader',
          'css-loader',
        ],
      },
    ],
  },
  plugins: [
    new HtmlWebpackPlugin({
      title: 'MAX',
      template: 'web_src/index.html'
    }),
    new VersionFile({
      output: './data/version.json',
      template: 'web_src/version.ejs'
    })
  ]
};