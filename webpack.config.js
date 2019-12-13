const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin')

module.exports = env => {
  return {
    entry: './web_src/index.js',
    output: {
      filename: 'main.js',
      path: path.resolve(__dirname, env.production ? 'web_src/progmem' : 'data'),
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
      })
    ]

  }
};