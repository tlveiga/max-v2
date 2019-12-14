const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin')
const CopyPlugin = require('copy-webpack-plugin');

module.exports = env => {
  return {
    entry: './web_src/index.js',
    output: {
      filename: 'main.js',
      path: path.resolve(__dirname, env.production ? 'progmem/cfg' : 'data'),
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
      new CopyPlugin([
        { from: 'web_src/operation.html', to: '../opt/NOEXT_index.html' }
      ])
    ]

  }
};